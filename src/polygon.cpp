#include "fixture/geometry.hpp"
#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>

namespace fixture {
namespace {
constexpr double eps=1e-9;
double cross(Vec2 a,Vec2 b,Vec2 c){return (b.x-a.x)*(c.y-a.y)-(b.y-a.y)*(c.x-a.x);}
double distance(Vec2 a,Vec2 b){return std::hypot(a.x-b.x,a.y-b.y);}
Vec2 closest(Vec2 a,Vec2 b,Vec2 p){
    const double dx=b.x-a.x,dy=b.y-a.y;
    const double t=std::clamp(((p.x-a.x)*dx+(p.y-a.y)*dy)/(dx*dx+dy*dy),0.0,1.0);
    return {a.x+t*dx,a.y+t*dy};
}
bool on_segment(Vec2 a,Vec2 b,Vec2 p){return std::abs(cross(a,b,p))<=eps && p.x>=std::min(a.x,b.x)-eps && p.x<=std::max(a.x,b.x)+eps && p.y>=std::min(a.y,b.y)-eps && p.y<=std::max(a.y,b.y)+eps;}
bool intersects(Vec2 a,Vec2 b,Vec2 c,Vec2 d){
    const double x=cross(a,b,c),y=cross(a,b,d),z=cross(c,d,a),w=cross(c,d,b);
    if(((x>eps&&y<-eps)||(x<-eps&&y>eps))&&((z>eps&&w<-eps)||(z<-eps&&w>eps)))return true;
    return on_segment(a,b,c)||on_segment(a,b,d)||on_segment(c,d,a)||on_segment(c,d,b);
}
}
std::vector<Vec2> boundary(const Scene& s){return s.outline.empty()?std::vector<Vec2>{{0,0},{s.plate.width,0},{s.plate.width,s.plate.height},{0,s.plate.height}}:s.outline;}
double polygon_area(const std::vector<Vec2>& p){
    double sum=0;for(std::size_t i=0;i<p.size();++i){const auto a=p[i],b=p[(i+1)%p.size()];sum+=a.x*b.y-a.y*b.x;}return std::abs(sum)*0.5;
}
void validate_polygon(const std::vector<Vec2>& p){
    if(p.size()<3||p.size()>2048)throw std::invalid_argument("outline needs 3..2048 vertices");
    for(std::size_t i=0;i<p.size();++i){
        const auto a=p[i],b=p[(i+1)%p.size()],c=p[(i+2)%p.size()];
        if(!std::isfinite(a.x)||!std::isfinite(a.y)||std::abs(a.x)>1e6||std::abs(a.y)>1e6)throw std::invalid_argument("outline coordinates must be finite and within +/-1e6 mm");
        if(distance(a,b)<1e-6)throw std::invalid_argument("zero-length outline edge");
        if(std::abs(cross(a,b,c))<eps && (b.x-a.x)*(c.x-b.x)+(b.y-a.y)*(c.y-b.y)<0)throw std::invalid_argument("backtracking outline edge");
        for(std::size_t j=i+1;j<p.size();++j){
            if(j==i+1||(i==0&&j==p.size()-1))continue;
            if(intersects(a,b,p[j],p[(j+1)%p.size()]))throw std::invalid_argument("self-intersecting or touching outline");
        }
    }
    if(polygon_area(p)<1e-6)throw std::invalid_argument("degenerate outline area");
}
bool point_inside(const std::vector<Vec2>& p,Vec2 q){
    bool inside=false;
    for(std::size_t i=0,j=p.size()-1;i<p.size();j=i++){
        if(on_segment(p[j],p[i],q))return true;
        if((p[i].y>q.y)!=(p[j].y>q.y) && q.x<(p[j].x-p[i].x)*(q.y-p[i].y)/(p[j].y-p[i].y)+p[i].x)inside=!inside;
    }return inside;
}
Vec2 closest_boundary(const Scene& s,Vec2 q){
    const auto p=boundary(s);double best=std::numeric_limits<double>::infinity();Vec2 result;
    for(std::size_t i=0;i<p.size();++i){auto c=closest(p[i],p[(i+1)%p.size()],q);double d=distance(c,q);if(d<best){best=d;result=c;}}return result;
}
double edge_clearance(const Scene& s,const Hole& h){
    const double d=distance(closest_boundary(s,h.centre),h.centre);
    return (point_inside(boundary(s),h.centre)?d:-d)-h.radius;
}
std::vector<Vec2> triangulate_material(const Scene& s,int segments){
    validate(s);
    if(segments<12||segments>512)throw std::invalid_argument("circle tessellation needs 12..512 segments");
    if(s.holes.size()>256)throw std::invalid_argument("viewer tessellation supports up to 256 holes; use CLI for larger drawings");
    if(boundary(s).size()+s.holes.size()*std::size_t(segments)>8192)throw std::invalid_argument("viewer contour budget exceeds 8192 vertices");
    // Invalid cutouts cannot define a simple material domain. Keep diagnostics drawable.
    for(const auto& h:s.holes)if(edge_clearance(s,h)<=eps)return {};
    for(std::size_t i=0;i<s.holes.size();++i)for(std::size_t j=i+1;j<s.holes.size();++j)if(hole_clearance(s.holes[i],s.holes[j])<=eps)return {};
    std::vector<std::vector<Vec2>> contours{boundary(s)};
    for(const auto& h:s.holes){std::vector<Vec2> ring;for(int i=0;i<segments;++i){double a=i*2*std::acos(-1.0)/segments;ring.push_back({h.centre.x+h.radius*std::cos(a),h.centre.y+h.radius*std::sin(a)});}contours.push_back(ring);}
    struct Edge{Vec2 a,b;double y(double x)const{return a.y+(x-a.x)*(b.y-a.y)/(b.x-a.x);}};
    std::vector<Edge> edges;std::vector<double> xs;
    for(const auto& ring:contours)for(std::size_t i=0;i<ring.size();++i){auto a=ring[i],b=ring[(i+1)%ring.size()];xs.push_back(a.x);if(std::abs(a.x-b.x)>eps)edges.push_back({a,b});}
    std::sort(xs.begin(),xs.end());xs.erase(std::unique(xs.begin(),xs.end(),[](double a,double b){return std::abs(a-b)<eps;}),xs.end());
    std::vector<Vec2> triangles;
    for(std::size_t i=1;i<xs.size();++i){
        const double l=xs[i-1],r=xs[i],mid=(l+r)*0.5;
        std::vector<const Edge*> active;for(const auto& e:edges)if(mid>std::min(e.a.x,e.b.x)&&mid<std::max(e.a.x,e.b.x))active.push_back(&e);
        std::sort(active.begin(),active.end(),[&](const Edge* a,const Edge* b){return a->y(mid)<b->y(mid);});
        if(active.size()%2)throw std::runtime_error("odd scanline parity; invalid contour topology");
        for(std::size_t j=0;j<active.size();j+=2){Vec2 a{l,active[j]->y(l)},b{r,active[j]->y(r)},c{r,active[j+1]->y(r)},d{l,active[j+1]->y(l)};
            if(std::abs(cross(a,b,c))>eps)triangles.insert(triangles.end(),{a,b,c});
            if(std::abs(cross(a,c,d))>eps)triangles.insert(triangles.end(),{a,c,d});
        }
    }return triangles;
}
} // namespace fixture
