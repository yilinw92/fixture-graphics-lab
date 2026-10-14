#include "fixture/geometry.hpp"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <random>
#include <set>
#include <stdexcept>

int main(){
    int checks=0;auto check=[&](bool ok,const char* label){++checks;if(!ok)throw std::runtime_error(label);};
    try{
        auto scene=fixture::read_dxf("examples/notched_fixture.dxf");
        check(scene.outline.size()==8&&scene.holes.size()==6,"DXF entity parsing");
        check(fixture::analyze(scene).empty(),"concave drawing passes");
        check(std::abs(fixture::polygon_area(scene.outline)-10850)<1e-8,"concave area");
        auto fail=fixture::read_dxf("examples/notch_violation.dxf");
        check(!fixture::analyze(fail).empty(),"notch clearance catches hole missed by bounding rectangle");
        check(fixture::edge_clearance(fail.plate,fail.holes.back())>3,"bounding-box check would incorrectly pass");
        auto inch=fixture::read_dxf("examples/inch_fixture.dxf");
        check(std::abs(inch.plate.width-50.8)<1e-10&&std::abs(inch.holes[0].centre.x-12.7)<1e-10,"inch conversion and origin normalization");
        for(const auto* name:{"invalid_open","invalid_bulge","invalid_crossing","invalid_units"}){bool rejected=false;try{fixture::read_dxf(std::string("examples/")+name+".dxf");}catch(const std::exception&){rejected=true;}check(rejected,"invalid CAD rejected");}
        auto reversed=scene;std::reverse(reversed.outline.begin(),reversed.outline.end());
        check(std::abs(fixture::metrics(scene).net_area_mm2-fixture::metrics(reversed).net_area_mm2)<1e-8,"winding invariant");
        const auto mesh=fixture::triangulate_material(scene,96);double mesh_area=0;
        for(std::size_t i=0;i<mesh.size();i+=3){const auto a=mesh[i],b=mesh[i+1],c=mesh[i+2];mesh_area+=std::abs((b.x-a.x)*(c.y-a.y)-(b.y-a.y)*(c.x-a.x))*0.5;
            fixture::Vec2 centroid{(a.x+b.x+c.x)/3,(a.y+b.y+c.y)/3};check(fixture::point_inside(scene.outline,centroid),"triangles stay inside concave boundary");
            for(auto h:scene.holes)check(std::hypot(centroid.x-h.centre.x,centroid.y-h.centre.y)>=h.radius*std::cos(std::acos(-1.0)/96)-1e-8,"triangles exclude hole interior");
        }
        double chord_area=fixture::polygon_area(scene.outline);for(auto h:scene.holes)chord_area-=96*0.5*h.radius*h.radius*std::sin(2*std::acos(-1.0)/96);
        check(std::abs(mesh_area-chord_area)<1e-6,"mesh area equals analytic polygonal material domain");
        std::filesystem::create_directories("artifacts");fixture::write_dxf(scene,"artifacts/roundtrip.dxf");
        check(std::abs(fixture::metrics(fixture::read_dxf("artifacts/roundtrip.dxf")).net_area_mm2-fixture::metrics(scene).net_area_mm2)<1e-8,"DXF round trip");
        // Differential oracle: independent brute-force pair set, fixed random seed.
        std::mt19937 rng(2026);std::uniform_real_distribution<double> pos(5,195),radius(1,7);
        for(int trial=0;trial<200;++trial){fixture::Scene random{{200,200,2},{0,3},{},{}};for(int i=0;i<40;++i)random.holes.push_back({i,{pos(rng),pos(rng)},radius(rng)});
            std::set<std::pair<int,int>> brute,indexed;for(int i=0;i<40;++i)for(int j=i+1;j<40;++j)if(fixture::hole_clearance(random.holes[i],random.holes[j])+1e-9<3)brute.emplace(i,j);
            fixture::AnalysisStats stats;for(const auto& f:fixture::analyze_indexed(random,stats))if(f.kind==fixture::FindingKind::HoleClearance)indexed.emplace(f.first,f.second);
            check(brute==indexed,"sweep-and-prune matches brute-force oracle");
        }
        fixture::Scene large{{1000,1000,2},{3,3},{},{}};for(int y=0;y<100;++y)for(int x=0;x<100;++x)large.holes.push_back({y*100+x,{5.0+x*10,5.0+y*10},1});
        fixture::AnalysisStats stats;auto start=std::chrono::steady_clock::now();auto found=fixture::analyze_indexed(large,stats);double ms=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-start).count();
        check(found.empty()&&stats.possible_pairs==49995000&&stats.narrow_phase_pairs==0,"sparse 10000-hole broad-phase workload");
        std::ofstream report("artifacts/cad_benchmark.json");report<<"{\"holes\":10000,\"possible_pairs\":"<<stats.possible_pairs<<",\"narrow_phase_pairs\":"<<stats.narrow_phase_pairs<<",\"wall_ms\":"<<ms<<",\"randomized_oracle_scenes\":200,\"mesh_triangles\":"<<mesh.size()/3<<"}\n";
        std::cout<<"PASS: CAD topology, tessellation, DXF roundtrip, 200 differential scenes; "<<checks<<" assertions. 10000-hole sweep: "<<ms<<" ms\n";
        return 0;
    }catch(const std::exception& e){std::cerr<<"FAIL: "<<e.what()<<'\n';return 1;}
}
