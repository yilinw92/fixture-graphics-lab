#pragma once
#include <string>
#include <vector>

namespace fixture {
struct Vec2 { double x{}, y{}; };
struct Hole { int id{}; Vec2 centre; double radius{}; };
struct Plate { double width{}, height{}, thickness{}; };
struct Rules { double edge_clearance{3.0}, web_clearance{3.0}; };
struct Scene { Plate plate; Rules rules; std::vector<Hole> holes; std::vector<Vec2> outline; };
enum class FindingKind { EdgeClearance, HoleClearance };
struct Finding {
    FindingKind kind;
    int first{}, second{-1};
    double actual{}, required{};
    std::string description;
};
struct Metrics { double net_area_mm2{}, cut_length_mm{}, volume_mm3{}; };
struct Rates { double material_per_mm2{0.002}, cut_per_mm{0.03}, setup{12.0}; };
struct Quote { int quantity{}; double material{}, cutting{}, setup{}, total{}; };

void validate(const Scene& scene);
double edge_clearance(const Plate& plate, const Hole& hole);
double hole_clearance(const Hole& a, const Hole& b);
std::vector<Finding> analyze(const Scene& scene);
struct AnalysisStats { std::size_t possible_pairs{}, narrow_phase_pairs{}; };
std::vector<Finding> analyze_indexed(const Scene& scene, AnalysisStats& stats);
std::vector<Vec2> boundary(const Scene& scene);
double polygon_area(const std::vector<Vec2>& polygon);
bool point_inside(const std::vector<Vec2>& polygon, Vec2 point);
Vec2 closest_boundary(const Scene& scene, Vec2 point);
double edge_clearance(const Scene& scene, const Hole& hole);
void validate_polygon(const std::vector<Vec2>& polygon);
// Vertical decomposition of a simple boundary and disjoint circular cutouts.
// Three consecutive vertices form one triangle; circles use a bounded chord error.
std::vector<Vec2> triangulate_material(const Scene& scene, int circle_segments = 96);
Scene read_dxf(const std::string& path, double thickness = 2.0);
void write_dxf(const Scene& scene, const std::string& path);
Metrics metrics(const Scene& scene);
Quote quote(const Scene& scene, int quantity, const Rates& rates = {});
Scene read_scene(const std::string& path);
Scene example_scene(int mode);
void export_report(const Scene& scene, const std::string& directory, int quantity = 10);

// Engineering coordinates: +X right, +Y up, all geometry in millimetres.
struct View2D {
    Vec2 centre{60, 40};
    double span_x{160};
    Vec2 world_to_screen(Vec2 world, int width, int height) const;
    Vec2 screen_to_world(Vec2 pixel, int width, int height) const;
    void zoom_at(Vec2 pixel, double factor, int width, int height);
};
int pick_hole(const Scene& scene, Vec2 world);
} // namespace fixture
