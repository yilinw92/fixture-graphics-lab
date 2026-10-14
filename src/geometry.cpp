#include "fixture/geometry.hpp"
#include <algorithm>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <numeric>
#include <set>
#include <sstream>
#include <stdexcept>

namespace fixture {
namespace {
constexpr double pi = 3.14159265358979323846;
constexpr double tolerance = 1e-9;
void finite_positive(double value, const char* label) {
    if (!std::isfinite(value) || value <= 0) throw std::invalid_argument(label);
}
void valid_view(const View2D& view, int width, int height) {
    finite_positive(view.span_x, "invalid view span");
    if (width <= 0 || height <= 0 || !std::isfinite(view.centre.x) || !std::isfinite(view.centre.y))
        throw std::invalid_argument("invalid viewport");
}
}

void validate(const Scene& scene) {
    finite_positive(scene.plate.width, "plate width must be positive and finite");
    finite_positive(scene.plate.height, "plate height must be positive and finite");
    finite_positive(scene.plate.thickness, "plate thickness must be positive and finite");
    if(std::max({scene.plate.width,scene.plate.height,scene.plate.thickness})>1e6)throw std::invalid_argument("blank dimensions exceed 1e6 mm bound");
    validate_polygon(boundary(scene));
    for(auto p:scene.outline)if(p.x < -tolerance||p.y < -tolerance||p.x>scene.plate.width+tolerance||p.y>scene.plate.height+tolerance)
        throw std::invalid_argument("outline must fit normalized blank bounds");
    if (!std::isfinite(scene.rules.edge_clearance) || scene.rules.edge_clearance < 0 ||
        !std::isfinite(scene.rules.web_clearance) || scene.rules.web_clearance < 0)
        throw std::invalid_argument("clearance rules must be finite and nonnegative");
    std::set<int> ids;
    for (const auto& hole : scene.holes) {
        finite_positive(hole.radius, "hole radius must be positive and finite");
        if (!std::isfinite(hole.centre.x) || !std::isfinite(hole.centre.y))
            throw std::invalid_argument("hole centre must be finite");
        if(std::max({std::abs(hole.centre.x),std::abs(hole.centre.y),hole.radius})>1e6)throw std::invalid_argument("hole exceeds coordinate/radius bound");
        if (!ids.insert(hole.id).second) throw std::invalid_argument("duplicate hole id");
    }
}

double edge_clearance(const Plate& plate, const Hole& hole) {
    return std::min({hole.centre.x, hole.centre.y, plate.width-hole.centre.x, plate.height-hole.centre.y}) - hole.radius;
}
double hole_clearance(const Hole& a, const Hole& b) {
    return std::hypot(a.centre.x-b.centre.x, a.centre.y-b.centre.y) - a.radius - b.radius;
}

std::vector<Finding> analyze(const Scene& scene) {
    AnalysisStats stats;
    return analyze_indexed(scene,stats);
}
std::vector<Finding> analyze_indexed(const Scene& scene, AnalysisStats& stats) {
    validate(scene);
    stats={scene.holes.size()*(scene.holes.size()-1)/2,0};
    std::vector<Finding> result;
    for (const auto& hole : scene.holes) {
        const double actual = edge_clearance(scene, hole);
        if (actual + tolerance < scene.rules.edge_clearance)
            result.push_back({FindingKind::EdgeClearance, hole.id, -1, actual,
                              scene.rules.edge_clearance, "Hole-to-plate-edge clearance below rule"});
    }
    // Sweep expanded X intervals; reject Y-separated candidates before hypot.
    std::vector<std::size_t> order(scene.holes.size()),active;
    std::iota(order.begin(),order.end(),0);
    auto left=[&](std::size_t i){return scene.holes[i].centre.x-scene.holes[i].radius;};
    std::sort(order.begin(),order.end(),[&](auto a,auto b){return left(a)<left(b);});
    for(auto index:order){
        const auto& h=scene.holes[index];
        active.erase(std::remove_if(active.begin(),active.end(),[&](auto i){return scene.holes[i].centre.x+scene.holes[i].radius+scene.rules.web_clearance+tolerance<left(index);}),active.end());
        for(auto other:active){
            const std::size_t i=std::min(index,other),j=std::max(index,other);
            if(std::abs(h.centre.y-scene.holes[other].centre.y)>h.radius+scene.holes[other].radius+scene.rules.web_clearance+tolerance)continue;
            ++stats.narrow_phase_pairs;
            const double actual = hole_clearance(scene.holes[i], scene.holes[j]);
            if (actual + tolerance < scene.rules.web_clearance)
                result.push_back({FindingKind::HoleClearance, scene.holes[i].id, scene.holes[j].id,
                                  actual, scene.rules.web_clearance, "Hole-to-hole web clearance below rule"});
        }
        active.push_back(index);
    }
    return result;
}

Metrics metrics(const Scene& scene) {
    validate(scene);
    const auto outline=boundary(scene);
    double area = polygon_area(outline),length=0;
    for(std::size_t i=0;i<outline.size();++i){auto a=outline[i],b=outline[(i+1)%outline.size()];length+=std::hypot(a.x-b.x,a.y-b.y);}
    for (const auto& hole : scene.holes) { area -= pi*hole.radius*hole.radius; length += 2*pi*hole.radius; }
    // These physical metrics require valid, contained, non-overlapping holes.
    if (!analyze(scene).empty()) throw std::invalid_argument("metrics require geometry that passes the configured rules");
    return {area, length, area*scene.plate.thickness};
}

Quote quote(const Scene& scene, int quantity, const Rates& rates) {
    if (quantity < 1) throw std::invalid_argument("quantity must be at least one");
    for (double v : {rates.material_per_mm2, rates.cut_per_mm, rates.setup})
        if (!std::isfinite(v) || v < 0) throw std::invalid_argument("rates must be finite and nonnegative");
    const auto m = metrics(scene);
    // Material is purchased as the rectangular blank; removed holes are not free stock.
    const double material = scene.plate.width*scene.plate.height*rates.material_per_mm2*quantity;
    const double cutting = m.cut_length_mm*rates.cut_per_mm*quantity;
    return {quantity, material, cutting, rates.setup, material+cutting+rates.setup};
}

Scene read_scene(const std::string& path) {
    std::ifstream input(path);
    if (!input) throw std::runtime_error("cannot open scene: " + path);
    Scene scene;
    bool got_plate = false, got_rules = false;
    std::string line; int line_number = 0;
    while (std::getline(input, line)) {
        ++line_number;
        line = line.substr(0, line.find('#'));
        std::istringstream row(line);
        std::string kind;
        if (!(row >> kind)) continue;
        bool ok = false;
        if (kind == "plate" && !got_plate) { ok = bool(row >> scene.plate.width >> scene.plate.height >> scene.plate.thickness); got_plate = true; }
        else if (kind == "rules" && !got_rules) { ok = bool(row >> scene.rules.edge_clearance >> scene.rules.web_clearance); got_rules = true; }
        else if (kind == "hole") { Hole hole; ok = bool(row >> hole.id >> hole.centre.x >> hole.centre.y >> hole.radius); if (ok) scene.holes.push_back(hole); }
        else if (kind == "vertex") { Vec2 p; ok=bool(row>>p.x>>p.y); if(ok)scene.outline.push_back(p); }
        std::string extra;
        if (!ok || (row >> extra)) throw std::invalid_argument("malformed scene at line " + std::to_string(line_number));
    }
    if (!got_plate) throw std::invalid_argument("scene needs one plate record");
    validate(scene);
    return scene;
}

Scene example_scene(int mode) {
    Scene scene{{120,80,2}, {3,3}, {{1,{18,18},5}, {2,{102,18},5}, {3,{18,62},5}, {4,{102,62},5}, {5,{60,40},8}}, {}};
    if (mode == 2) scene.holes[0].centre.x = 6;
    if (mode == 3) scene.holes[4].centre = {27,18};
    return scene;
}

void export_report(const Scene& scene, const std::string& directory, int quantity) {
    validate(scene);
    if (quantity < 1) throw std::invalid_argument("quantity must be at least one");
    AnalysisStats stats;const auto findings = analyze_indexed(scene,stats);
    std::filesystem::create_directories(directory);
    std::ofstream json(std::filesystem::path(directory)/"report.json");
    std::ofstream csv(std::filesystem::path(directory)/"findings.csv");
    std::ofstream svg(std::filesystem::path(directory)/"preview.svg");
    if (!json || !csv || !svg) throw std::runtime_error("cannot write report directory");
    json << std::fixed << std::setprecision(6);
    json << "{\n  \"units\": \"mm\",\n  \"synthetic_demo_rates\": true,\n  \"possible_pairs\": "<<stats.possible_pairs<<",\n  \"narrow_phase_pairs\": "<<stats.narrow_phase_pairs<<",\n  \"outline_vertices\": "<<boundary(scene).size()<<",\n  \"geometry_passes\": " << (findings.empty()?"true":"false") << ",\n  \"findings\": [";
    csv << "kind,hole_a,hole_b,actual_mm,required_mm\n";
    for (std::size_t i=0; i<findings.size(); ++i) {
        const auto& f = findings[i];
        const std::string kind = f.kind==FindingKind::EdgeClearance?"edge":"web";
        json << (i?",":"") << "{\"kind\":\"" << kind << "\",\"hole_a\":" << f.first << ",\"hole_b\":" << f.second << ",\"actual_mm\":" << f.actual << ",\"required_mm\":" << f.required << "}";
        csv << kind << ',' << f.first << ',' << f.second << ',' << f.actual << ',' << f.required << '\n';
    }
    json << "],\n  \"quote\": ";
    if (findings.empty()) {
        const auto q=quote(scene,quantity); const auto m=metrics(scene);
        json << "{\"quantity\":" << q.quantity << ",\"material\":" << q.material << ",\"cutting\":" << q.cutting << ",\"setup\":" << q.setup << ",\"total\":" << q.total << "},\n  \"net_area_mm2\":" << m.net_area_mm2 << ",\n  \"cut_length_mm\":" << m.cut_length_mm;
    } else json << "null";
    json << "\n}\n";
    svg << "<svg xmlns=\"http://www.w3.org/2000/svg\" viewBox=\"-10 -10 " << scene.plate.width+20 << ' ' << scene.plate.height+20 << "\"><rect x=\"-10\" y=\"-10\" width=\"100%\" height=\"100%\" fill=\"#101827\"/><polygon points=\"";
    for(auto p:boundary(scene))svg<<p.x<<','<<scene.plate.height-p.y<<' ';
    svg<<"\" fill=\"#415d7b\" stroke=\"#70b7f2\" stroke-width=\"0.4\"/>";
    for (const auto& h : scene.holes) {
        const bool bad=std::any_of(findings.begin(),findings.end(),[&](const auto& f){return f.first==h.id||f.second==h.id;});
        svg << "<circle cx=\"" << h.centre.x << "\" cy=\"" << scene.plate.height-h.centre.y << "\" r=\"" << h.radius << "\" fill=\"#101827\" stroke=\"" << (bad?"#ff7169":"#74dfce") << "\" stroke-width=\"0.6\"/><text x=\"" << h.centre.x+h.radius+1 << "\" y=\"" << scene.plate.height-h.centre.y << "\" fill=\"white\" font-size=\"3\">H" << h.id << "</text>";
    }
    svg << "</svg>\n";
    write_dxf(scene,(std::filesystem::path(directory)/"normalized.dxf").string());
    if (!json || !csv || !svg) throw std::runtime_error("report write failed");
}

Vec2 View2D::world_to_screen(Vec2 world, int width, int height) const {
    valid_view(*this,width,height);
    const double span_y=span_x*height/width;
    return {(world.x-centre.x)/span_x*width+width/2.0, height/2.0-(world.y-centre.y)/span_y*height};
}
Vec2 View2D::screen_to_world(Vec2 pixel, int width, int height) const {
    valid_view(*this,width,height);
    const double span_y=span_x*height/width;
    return {centre.x+(pixel.x-width/2.0)*span_x/width, centre.y-(pixel.y-height/2.0)*span_y/height};
}
void View2D::zoom_at(Vec2 pixel,double factor,int width,int height) {
    finite_positive(factor,"zoom factor must be positive");
    const auto before=screen_to_world(pixel,width,height);
    span_x=std::clamp(span_x/factor,10.0,5000.0);
    const auto after=screen_to_world(pixel,width,height);
    centre.x+=before.x-after.x; centre.y+=before.y-after.y;
}
int pick_hole(const Scene& scene,Vec2 world) {
    for (int i=int(scene.holes.size())-1;i>=0;--i)
        if (std::hypot(scene.holes[i].centre.x-world.x,scene.holes[i].centre.y-world.y)<=scene.holes[i].radius) return i;
    return -1;
}
} // namespace fixture
