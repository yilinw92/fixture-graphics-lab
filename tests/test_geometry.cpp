#include "fixture/geometry.hpp"
#include <cmath>
#include <functional>
#include <iostream>
#include <limits>
#include <stdexcept>

int main() {
    int count=0;
    auto check=[&](bool condition,const char* label){++count;if(!condition) throw std::runtime_error(label);};
    auto rejects=[&](const std::function<void()>& action,const char* label){bool raised=false;try{action();}catch(const std::exception&){raised=true;}check(raised,label);};
    try {
        const auto nominal=fixture::example_scene(1);
        check(fixture::analyze(nominal).empty(),"nominal scene");
        check(fixture::analyze(fixture::example_scene(2)).size()==1,"edge violation");
        check(!fixture::analyze(fixture::example_scene(3)).empty(),"overlap violation");
        auto scene=nominal; scene.holes[0].centre.x=8;
        check(fixture::analyze(scene).empty(),"boundary equality passes");
        scene.holes[0].centre.x=7.99;
        check(!fixture::analyze(scene).empty(),"just below clearance fails");
        scene=nominal;scene.holes[0].centre.x=-1;
        check(!fixture::analyze(scene).empty(),"outside plate fails");
        scene=nominal;scene.holes[0].radius=-1;
        rejects([&]{fixture::validate(scene);},"negative radius");
        scene=nominal;scene.holes[0].centre.x=std::numeric_limits<double>::quiet_NaN();
        rejects([&]{fixture::validate(scene);},"NaN rejected");
        scene=nominal;scene.plate.width=std::numeric_limits<double>::infinity();
        rejects([&]{fixture::validate(scene);},"infinity rejected");
        scene=nominal;scene.holes[1].id=scene.holes[0].id;
        rejects([&]{fixture::validate(scene);},"duplicate id");
        const auto m=fixture::metrics(nominal);
        check(std::abs(m.net_area_mm2-(9600-164*std::acos(-1.0)))<1e-8,"analytic area");
        check(std::abs(m.cut_length_mm-(400+56*std::acos(-1.0)))<1e-8,"analytic cut length");
        const auto q=fixture::quote(nominal,10);
        check(std::abs(q.material-192)<1e-8,"blank material cost");
        check(std::abs(q.total-(q.material+q.cutting+q.setup))<1e-8,"itemized quote");
        rejects([&]{fixture::quote(nominal,0);},"zero quantity");
        rejects([&]{fixture::quote(fixture::example_scene(2),10);},"invalid geometry not quoted");
        fixture::View2D view; const fixture::Vec2 world{18,62};
        const auto screen=view.world_to_screen(world,1200,700); const auto back=view.screen_to_world(screen,1200,700);
        check(std::hypot(back.x-world.x,back.y-world.y)<1e-10,"transform round trip");
        view.zoom_at(screen,1.7,1200,700);const auto anchored=view.screen_to_world(screen,1200,700);
        check(std::hypot(anchored.x-world.x,anchored.y-world.y)<1e-10,"cursor anchored zoom");
        check(fixture::pick_hole(nominal,{18,18})==0,"analytic picking");
        check(fixture::pick_hole(nominal,{40,60})==-1,"empty-space picking");
        std::cout<<"PASS: "<<count<<" geometry, quoting, input and transform checks.\n";
        return 0;
    }catch(const std::exception& error){std::cerr<<"FAIL: "<<error.what()<<'\n';return 1;}
}
