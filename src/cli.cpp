#include "fixture/geometry.hpp"
#include <iostream>
#include <stdexcept>
#include <string>

int main(int argc,char** argv) {
    try {
        fixture::Scene scene=fixture::example_scene(1);
        std::string output="artifacts/report";
        int quantity=10;
        for(int i=1;i<argc;++i) {
            const std::string arg=argv[i];
            if(arg=="--scene" && i+1<argc) scene=fixture::read_scene(argv[++i]);
            else if(arg=="--out" && i+1<argc) output=argv[++i];
            else if(arg=="--quantity" && i+1<argc) {
                const std::string raw=argv[++i]; std::size_t used=0;
                quantity=std::stoi(raw,&used); if(used!=raw.size()) throw std::invalid_argument("bad quantity");
            } else if(arg=="--help") {
                std::cout<<"fixture_cli [--scene examples/nominal.scene] [--out artifacts/report] [--quantity 10]\n"; return 0;
            } else throw std::invalid_argument("unknown or incomplete argument: "+arg);
        }
        fixture::export_report(scene,output,quantity);
        const auto findings=fixture::analyze(scene);
        std::cout<<"Geometry findings: "<<findings.size()<<"\n";
        for(const auto& f:findings) std::cout<<"H"<<f.first<<": "<<f.description<<" ("<<f.actual<<" mm, needs "<<f.required<<" mm)\n";
        if(findings.empty()) {
            const auto q=fixture::quote(scene,quantity);
            std::cout<<"Synthetic quote total: "<<q.total<<" for "<<quantity<<" parts\n";
        }
        std::cout<<"Reports: "<<output<<"/{report.json,findings.csv,preview.svg}\n";
        return findings.empty()?0:2;
    } catch(const std::exception& error) {std::cerr<<"Error: "<<error.what()<<'\n';return 1;}
}
