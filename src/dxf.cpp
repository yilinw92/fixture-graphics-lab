#include "fixture/geometry.hpp"
#include <algorithm>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <map>
#include <sstream>
#include <stdexcept>

namespace fixture {
namespace {
struct Tag{int code;std::string value;int line;};
std::string trim(std::string s){auto a=s.find_first_not_of(" \t\r"),b=s.find_last_not_of(" \t\r");return a==std::string::npos?"":s.substr(a,b-a+1);}
double number(const Tag& t){
    try{std::size_t n=0;double v=std::stod(t.value,&n);if(n!=t.value.size()||!std::isfinite(v))throw std::invalid_argument("number");return v;}
    catch(const std::exception&){throw std::invalid_argument("DXF invalid number at line "+std::to_string(t.line));}
}
int integer(const Tag& t){double v=number(t);if(v!=std::floor(v)||std::abs(v)>1000000)throw std::invalid_argument("DXF invalid integer at line "+std::to_string(t.line));return int(v);}
}
Scene read_dxf(const std::string& path,double thickness){
    std::ifstream in(path);if(!in)throw std::runtime_error("cannot open DXF: "+path);
    std::vector<Tag> tags;std::string a,b;int line=0;
    while(std::getline(in,a)){
        ++line;if(!std::getline(in,b))throw std::invalid_argument("DXF incomplete group-code/value pair");++line;
        Tag code{0,trim(a),line-1};tags.push_back({integer(code),trim(b),line});
        if(tags.size()>200000)throw std::invalid_argument("DXF exceeds 200000 tags");
    }
    if(tags.empty()||tags.back().code!=0||tags.back().value!="EOF")throw std::invalid_argument("DXF missing EOF");
    double scale=0;std::string section;bool saw_entities=false;Scene s;s.plate.thickness=thickness;
    for(std::size_t i=0;i<tags.size();){
        const auto& t=tags[i];
        if(t.code==0&&t.value=="SECTION"){
            if(!section.empty()||i+1>=tags.size()||tags[i+1].code!=2)throw std::invalid_argument("DXF malformed SECTION");
            section=tags[i+1].value;if(section=="ENTITIES"){if(saw_entities)throw std::invalid_argument("DXF duplicate ENTITIES");saw_entities=true;}i+=2;continue;
        }
        if(t.code==0&&t.value=="ENDSEC"){section.clear();++i;continue;}
        if(section=="HEADER"&&t.code==9&&t.value=="$INSUNITS"){
            if(scale||i+1>=tags.size()||tags[i+1].code!=70)throw std::invalid_argument("DXF malformed/duplicate INSUNITS");
            int units=integer(tags[i+1]);if(units!=1&&units!=4)throw std::invalid_argument("DXF requires explicit inch (1) or millimetre (4) units");scale=units==1?25.4:1.0;i+=2;continue;
        }
        if(section!="ENTITIES"){++i;continue;}
        if(t.code!=0)throw std::invalid_argument("DXF expected entity at line "+std::to_string(t.line));
        if(t.value!="LWPOLYLINE"&&t.value!="CIRCLE")throw std::invalid_argument("unsupported DXF entity: "+t.value+" at line "+std::to_string(t.line));
        std::size_t end=i+1;while(end<tags.size()&&tags[end].code!=0)++end;
        std::map<int,Tag> fields;std::vector<Vec2> vertices;bool needs_y=false;int count=-1,flags=0;
        for(std::size_t j=i+1;j<end;++j){const auto& q=tags[j];
            if((q.code==42||q.code==38||q.code==39||q.code==30||q.code==41||q.code==43||q.code==210||q.code==220||(q.code==40&&t.value=="LWPOLYLINE"))&&number(q)!=0)throw std::invalid_argument("DXF curves, widths, elevation and tilted OCS are unsupported at line "+std::to_string(q.line));
            if(q.code==230&&number(q)!=1)throw std::invalid_argument("DXF requires +Z extrusion");
            if(t.value=="LWPOLYLINE"){
                if(q.code==90){if(count!=-1)throw std::invalid_argument("duplicate vertex count");count=integer(q);}
                if(q.code==70)flags=integer(q);
                if(q.code==10){if(needs_y)throw std::invalid_argument("vertex missing Y");vertices.push_back({number(q),0});needs_y=true;}
                if(q.code==20){if(!needs_y)throw std::invalid_argument("vertex missing X");vertices.back().y=number(q);needs_y=false;}
            }else if(q.code==10||q.code==20||q.code==40){if(!fields.emplace(q.code,q).second)throw std::invalid_argument("DXF duplicate circle field");}
        }
        if(t.value=="LWPOLYLINE"){
            if(!s.outline.empty())throw std::invalid_argument("DXF supports one outer closed polyline");
            if(!(flags&1)||count!=int(vertices.size())||needs_y)throw std::invalid_argument("DXF polyline must be closed with matching vertex count");
            s.outline=vertices;
        }else{
            if(fields.size()!=3)throw std::invalid_argument("DXF circle needs X, Y and radius");
            s.holes.push_back({int(s.holes.size()+1),{number(fields.at(10)),number(fields.at(20))},number(fields.at(40))});
            if(s.holes.size()>10000)throw std::invalid_argument("DXF exceeds 10000 circles");
        }i=end;
    }
    if(!section.empty()||!saw_entities||s.outline.empty()||!scale)throw std::invalid_argument("DXF needs HEADER/INSUNITS and one outer polyline");
    // Normalize drawing origin and units once; all downstream geometry is in mm.
    double minx=s.outline[0].x,miny=s.outline[0].y,maxx=minx,maxy=miny;
    for(auto p:s.outline){minx=std::min(minx,p.x);miny=std::min(miny,p.y);maxx=std::max(maxx,p.x);maxy=std::max(maxy,p.y);}
    for(auto& p:s.outline){p.x=(p.x-minx)*scale;p.y=(p.y-miny)*scale;}
    for(auto& h:s.holes){h.centre.x=(h.centre.x-minx)*scale;h.centre.y=(h.centre.y-miny)*scale;h.radius*=scale;}
    s.plate.width=(maxx-minx)*scale;s.plate.height=(maxy-miny)*scale;validate(s);return s;
}
void write_dxf(const Scene& s,const std::string& path){
    validate(s);std::ofstream out(path);if(!out)throw std::runtime_error("cannot write DXF");
    out<<std::setprecision(17)<<"0\nSECTION\n2\nHEADER\n9\n$INSUNITS\n70\n4\n0\nENDSEC\n0\nSECTION\n2\nENTITIES\n0\nLWPOLYLINE\n100\nAcDbEntity\n8\nOUTLINE\n100\nAcDbPolyline\n90\n"<<boundary(s).size()<<"\n70\n1\n";
    for(auto p:boundary(s))out<<"10\n"<<p.x<<"\n20\n"<<p.y<<'\n';
    for(auto h:s.holes)out<<"0\nCIRCLE\n100\nAcDbEntity\n8\nHOLES\n100\nAcDbCircle\n10\n"<<h.centre.x<<"\n20\n"<<h.centre.y<<"\n30\n0\n40\n"<<h.radius<<'\n';
    out<<"0\nENDSEC\n0\nEOF\n";if(!out)throw std::runtime_error("DXF write failed");
}
} // namespace fixture
