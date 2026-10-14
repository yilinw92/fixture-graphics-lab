#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#endif
#include <GL/gl.h>
#include "fixture/gl_api.hpp"
#include "fixture/geometry.hpp"
#include "fixture/window.hpp"
#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <thread>
#include <vector>

namespace {
struct Vertex {float x,y,r,g,b;};
using Colour=std::array<float,3>;
constexpr Colour background{0.04f,0.07f,0.12f},plate_colour{0.18f,0.32f,0.46f},good{0.35f,0.85f,0.75f},bad{1.0f,0.35f,0.30f};
void add(std::vector<Vertex>& v,fixture::Vec2 p,Colour c){v.push_back({float(p.x),float(p.y),c[0],c[1],c[2]});}
void line(std::vector<Vertex>& v,fixture::Vec2 a,fixture::Vec2 b,Colour c){add(v,a,c);add(v,b,c);}
void circle(std::vector<Vertex>& fill,std::vector<Vertex>& edges,fixture::Vec2 centre,double radius,Colour colour){
    (void)fill;
    constexpr int segments=96;constexpr double pi=3.14159265358979323846;
    for(int i=0;i<segments;++i){double a=i*2*pi/segments,b=(i+1)*2*pi/segments;fixture::Vec2 p{centre.x+radius*std::cos(a),centre.y+radius*std::sin(a)},q{centre.x+radius*std::cos(b),centre.y+radius*std::sin(b)};
        line(edges,p,q,colour);
    }
}

class Renderer {
    int cached_revision{-1};std::vector<Vertex> cached_material;
    PFNGLGENVERTEXARRAYSPROC gen_vao{};PFNGLBINDVERTEXARRAYPROC bind_vao{};PFNGLDELETEVERTEXARRAYSPROC delete_vao{};
    PFNGLGENBUFFERSPROC gen_buffers{};PFNGLBINDBUFFERPROC bind_buffer{};PFNGLBUFFERDATAPROC buffer_data{};PFNGLDELETEBUFFERSPROC delete_buffers{};
    PFNGLCREATESHADERPROC create_shader{};PFNGLSHADERSOURCEPROC shader_source{};PFNGLCOMPILESHADERPROC compile_shader{};PFNGLGETSHADERIVPROC shader_iv{};PFNGLGETSHADERINFOLOGPROC shader_log{};PFNGLDELETESHADERPROC delete_shader{};
    PFNGLCREATEPROGRAMPROC create_program{};PFNGLATTACHSHADERPROC attach_shader{};PFNGLLINKPROGRAMPROC link_program{};PFNGLGETPROGRAMIVPROC program_iv{};PFNGLGETPROGRAMINFOLOGPROC program_log{};PFNGLUSEPROGRAMPROC use_program{};PFNGLDELETEPROGRAMPROC delete_program{};
    PFNGLVERTEXATTRIBPOINTERPROC attrib_pointer{};PFNGLENABLEVERTEXATTRIBARRAYPROC enable_attrib{};PFNGLGETUNIFORMLOCATIONPROC uniform_location{};PFNGLUNIFORMMATRIX3FVPROC uniform_matrix{};
    GLuint vao{},vbo{},program{};GLint view_location{};
    template<class T>void load(T& ptr,const fixture::Window& w,const char* name){ptr=reinterpret_cast<T>(w.proc(name));if(!ptr)throw std::runtime_error(std::string("missing OpenGL function: ")+name);}
    GLuint shader(GLenum type,const char* source){GLuint id=create_shader(type);shader_source(id,1,&source,nullptr);compile_shader(id);GLint ok=0;shader_iv(id,GL_COMPILE_STATUS,&ok);if(!ok){char log[2048]{};shader_log(id,sizeof(log),nullptr,log);delete_shader(id);throw std::runtime_error(log);}return id;}
public:
    explicit Renderer(const fixture::Window& w){
#define LOAD(member,name) load(member,w,name)
        LOAD(gen_vao,"glGenVertexArrays");LOAD(bind_vao,"glBindVertexArray");LOAD(delete_vao,"glDeleteVertexArrays");
        LOAD(gen_buffers,"glGenBuffers");LOAD(bind_buffer,"glBindBuffer");LOAD(buffer_data,"glBufferData");LOAD(delete_buffers,"glDeleteBuffers");
        LOAD(create_shader,"glCreateShader");LOAD(shader_source,"glShaderSource");LOAD(compile_shader,"glCompileShader");LOAD(shader_iv,"glGetShaderiv");LOAD(shader_log,"glGetShaderInfoLog");LOAD(delete_shader,"glDeleteShader");
        LOAD(create_program,"glCreateProgram");LOAD(attach_shader,"glAttachShader");LOAD(link_program,"glLinkProgram");LOAD(program_iv,"glGetProgramiv");LOAD(program_log,"glGetProgramInfoLog");LOAD(use_program,"glUseProgram");LOAD(delete_program,"glDeleteProgram");
        LOAD(attrib_pointer,"glVertexAttribPointer");LOAD(enable_attrib,"glEnableVertexAttribArray");LOAD(uniform_location,"glGetUniformLocation");LOAD(uniform_matrix,"glUniformMatrix3fv");
#undef LOAD
        const char* vertex=R"(#version 330 core
layout(location=0) in vec2 position;
layout(location=1) in vec3 colour;
uniform mat3 view_matrix;
out vec3 vertex_colour;
void main(){vec3 p=view_matrix*vec3(position,1.0);gl_Position=vec4(p.xy,0.0,1.0);vertex_colour=colour;})";
        const char* fragment=R"(#version 330 core
in vec3 vertex_colour;
out vec4 output_colour;
void main(){output_colour=vec4(vertex_colour,1.0);})";
        GLuint vs=shader(GL_VERTEX_SHADER,vertex),fs=shader(GL_FRAGMENT_SHADER,fragment);
        program=create_program();attach_shader(program,vs);attach_shader(program,fs);link_program(program);delete_shader(vs);delete_shader(fs);
        GLint ok=0;program_iv(program,GL_LINK_STATUS,&ok);if(!ok){char log[2048]{};program_log(program,sizeof(log),nullptr,log);throw std::runtime_error(log);}
        gen_vao(1,&vao);gen_buffers(1,&vbo);bind_vao(vao);bind_buffer(GL_ARRAY_BUFFER,vbo);
        attrib_pointer(0,2,GL_FLOAT,GL_FALSE,sizeof(Vertex),nullptr);enable_attrib(0);
        attrib_pointer(1,3,GL_FLOAT,GL_FALSE,sizeof(Vertex),reinterpret_cast<void*>(2*sizeof(float)));enable_attrib(1);
        view_location=uniform_location(program,"view_matrix");
    }
    ~Renderer(){if(vbo)delete_buffers(1,&vbo);if(vao)delete_vao(1,&vao);if(program)delete_program(program);}
    void draw(const fixture::Scene& scene,const fixture::View2D& view,int width,int height,int selected,int revision){
        std::vector<Vertex> triangles,lines;const auto findings=fixture::analyze(scene);
        if(revision!=cached_revision){cached_material.clear();for(auto p:fixture::triangulate_material(scene))add(cached_material,p,plate_colour);cached_revision=revision;}
        triangles=cached_material;
        for(int i=-100;i<=250;i+=10){line(lines,{double(i),-100},{double(i),200},{0.1f,0.15f,0.22f});line(lines,{-100,double(i)},{250,double(i)},{0.1f,0.15f,0.22f});}
        const auto outline=fixture::boundary(scene);
        for(std::size_t i=0;i<outline.size();++i)line(lines,outline[i],outline[(i+1)%outline.size()],{0.45f,0.7f,0.95f});
        for(std::size_t i=0;i<scene.holes.size();++i){const auto& hole=scene.holes[i];const bool invalid=std::any_of(findings.begin(),findings.end(),[&](auto f){return f.first==hole.id||f.second==hole.id;});
            circle(triangles,lines,hole.centre,hole.radius,invalid?bad:good);
            if(int(i)==selected){std::vector<Vertex> discard;circle(discard,lines,hole.centre,hole.radius+1,{1.0f,0.85f,0.3f});}
        }
        for(const auto& finding:findings){auto it=std::find_if(scene.holes.begin(),scene.holes.end(),[&](auto h){return h.id==finding.first;});if(it==scene.holes.end())continue;
            if(finding.second>=0){auto other=std::find_if(scene.holes.begin(),scene.holes.end(),[&](auto h){return h.id==finding.second;});if(other!=scene.holes.end())line(lines,it->centre,other->centre,bad);}
            else line(lines,it->centre,fixture::closest_boundary(scene,it->centre),bad);
        }
        line(lines,{0,0},{15,0},{1,0.4f,0.4f});line(lines,{0,0},{0,15},{0.3f,1,0.4f});
        glViewport(0,0,width,height);glClearColor(background[0],background[1],background[2],1);glClear(GL_COLOR_BUFFER_BIT);
        use_program(program);bind_vao(vao);bind_buffer(GL_ARRAY_BUFFER,vbo);
        const float sx=float(2/view.span_x),sy=float(2/(view.span_x*height/width));
        const float matrix[]={sx,0,0,0,sy,0,float(-view.centre.x*sx),float(-view.centre.y*sy),1};uniform_matrix(view_location,1,GL_FALSE,matrix);
        for(auto [mode,data]:std::vector<std::pair<GLenum,const std::vector<Vertex>*>>{{GL_TRIANGLES,&triangles},{GL_LINES,&lines}}){buffer_data(GL_ARRAY_BUFFER,GLsizeiptr(data->size()*sizeof(Vertex)),data->data(),GL_DYNAMIC_DRAW);glDrawArrays(mode,0,GLsizei(data->size()));}
        if(glGetError()!=GL_NO_ERROR)throw std::runtime_error("OpenGL draw error");
    }
};

void capture(const std::string& path,int width,int height){
    std::vector<unsigned char> pixels(std::size_t(width)*height*3);
    glFinish();glPixelStorei(GL_PACK_ALIGNMENT,1);glReadBuffer(GL_BACK);glReadPixels(0,0,width,height,GL_RGB,GL_UNSIGNED_BYTE,pixels.data());
    if(glGetError()!=GL_NO_ERROR)throw std::runtime_error("OpenGL readback failed");
    const auto parent=std::filesystem::path(path).parent_path();if(!parent.empty())std::filesystem::create_directories(parent);
    std::ofstream out(path,std::ios::binary);out<<"P6\n"<<width<<' '<<height<<"\n255\n";
    for(int y=height-1;y>=0;--y)out.write(reinterpret_cast<const char*>(pixels.data()+std::size_t(y)*width*3),width*3);
    if(!out)throw std::runtime_error("cannot write capture");
}
}

int main(int argc,char** argv){
    try{
        std::string capture_path;fixture::Scene scene=fixture::example_scene(1);
        for(int i=1;i<argc;++i){std::string arg=argv[i];if(arg=="--capture" && i+1<argc)capture_path=argv[++i];else if(arg=="--example" && i+1<argc){int mode=std::stoi(argv[++i]);if(mode<1||mode>3)throw std::invalid_argument("example must be 1, 2, or 3");scene=fixture::example_scene(mode);}else if(arg=="--dxf"&&i+1<argc)scene=fixture::read_dxf(argv[++i]);else if(arg=="--scene"&&i+1<argc)scene=fixture::read_scene(argv[++i]);else throw std::invalid_argument("usage: fixture_viewer [--dxf file | --scene file] [--capture file.ppm] [--example 1|2|3]");}
        fixture::Window window(!capture_path.empty());Renderer renderer(window);fixture::View2D view;int selected=0,revision=0;
        std::vector<fixture::Scene> undo,redo;
        auto fit=[&](){view.centre={scene.plate.width/2,scene.plate.height/2};view.span_x=std::max(scene.plate.width,scene.plate.height*window.width/window.height)*1.4;};fit();
        std::cout<<"OpenGL: "<<glGetString(GL_VERSION)<<" | Renderer: "<<glGetString(GL_RENDERER)<<"\n";
        do{
            if(!window.poll())break;
            if(window.clicked){int hit=fixture::pick_hole(scene,view.screen_to_world({double(window.mouse_x),double(window.mouse_y)},window.width,window.height));if(hit>=0)selected=hit;}
            if(window.wheel)view.zoom_at({double(window.mouse_x),double(window.mouse_y)},std::pow(1.15,window.wheel),window.width,window.height);
            for(int key:window.keys){
                if((key>=37&&key<=40&&!scene.holes.empty())||(key>='1'&&key<='3')){undo.push_back(scene);if(undo.size()>100)undo.erase(undo.begin());redo.clear();++revision;}
                if(key>='1' && key<='3'){scene=fixture::example_scene(key-'0');selected=0;fit();}
                else if(key=='Z'&&!undo.empty()){redo.push_back(scene);scene=undo.back();undo.pop_back();selected=0;++revision;}
                else if(key=='Y'&&!redo.empty()){undo.push_back(scene);scene=redo.back();redo.pop_back();selected=0;++revision;}
                else if(key==37&&!scene.holes.empty())scene.holes[selected].centre.x-=0.5;
                else if(key==39&&!scene.holes.empty())scene.holes[selected].centre.x+=0.5;
                else if(key==38&&!scene.holes.empty())scene.holes[selected].centre.y+=0.5;
                else if(key==40&&!scene.holes.empty())scene.holes[selected].centre.y-=0.5;
                else if(key=='A')view.centre.x-=3;else if(key=='D')view.centre.x+=3;
                else if(key=='W')view.centre.y+=3;else if(key=='S')view.centre.y-=3;
                else if(key=='E')fixture::export_report(scene,"artifacts/viewer");
                else if(key=='R')fit();
            }
            window.title("DXF Fixture Inspector | Findings: "+std::to_string(fixture::analyze(scene).size())+" | "+(scene.holes.empty()?"No holes":"H"+std::to_string(scene.holes[selected].id))+" | arrows edit, Z/Y undo/redo, wheel zoom, E export DXF/report");
            renderer.draw(scene,view,window.width,window.height,selected,revision);
            if(!capture_path.empty()){capture(capture_path,window.width,window.height);std::cout<<"Capture: "<<capture_path<<"\n";break;}
            window.present();std::this_thread::sleep_for(std::chrono::milliseconds(16));
        }while(true);
        return 0;
    }catch(const std::exception& error){std::cerr<<"Error: "<<error.what()<<'\n';return 1;}
}
