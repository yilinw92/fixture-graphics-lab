#ifndef _WIN32
#include "fixture/window.hpp"
#include <GLFW/glfw3.h>
#include <stdexcept>

namespace fixture {
struct Window::Impl {GLFWwindow* window{};};
Window::Window(bool hidden){
    if(!glfwInit())throw std::runtime_error("GLFW initialization failed; for headless Linux use xvfb-run");
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR,3);glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR,3);
    glfwWindowHint(GLFW_OPENGL_PROFILE,GLFW_OPENGL_CORE_PROFILE);glfwWindowHint(GLFW_VISIBLE,hidden?GLFW_FALSE:GLFW_TRUE);
    auto* handle=glfwCreateWindow(width,height,"Fixture Graphics Lab",nullptr,nullptr);
    if(!handle){glfwTerminate();throw std::runtime_error("OpenGL 3.3 context unavailable");}
    impl_=new Impl{handle};glfwMakeContextCurrent(handle);glfwSetWindowUserPointer(handle,this);
    glfwSetFramebufferSizeCallback(handle,[](GLFWwindow* w,int x,int y){auto* p=static_cast<Window*>(glfwGetWindowUserPointer(w));p->width=x>0?x:1;p->height=y>0?y:1;});
    glfwSetCursorPosCallback(handle,[](GLFWwindow* w,double x,double y){auto* p=static_cast<Window*>(glfwGetWindowUserPointer(w));p->mouse_x=int(x);p->mouse_y=int(y);});
    glfwSetMouseButtonCallback(handle,[](GLFWwindow* w,int button,int action,int){if(button==GLFW_MOUSE_BUTTON_LEFT && action==GLFW_PRESS)static_cast<Window*>(glfwGetWindowUserPointer(w))->clicked=true;});
    glfwSetScrollCallback(handle,[](GLFWwindow* w,double,double y){static_cast<Window*>(glfwGetWindowUserPointer(w))->wheel+=y;});
    glfwSetKeyCallback(handle,[](GLFWwindow* w,int key,int,int action,int){
        if(action!=GLFW_PRESS && action!=GLFW_REPEAT)return;
        auto* p=static_cast<Window*>(glfwGetWindowUserPointer(w));
        if(key==GLFW_KEY_ESCAPE)p->closing=true;
        else {if(key==GLFW_KEY_LEFT)key=37;else if(key==GLFW_KEY_UP)key=38;else if(key==GLFW_KEY_RIGHT)key=39;else if(key==GLFW_KEY_DOWN)key=40;p->keys.push_back(key);}
    });
}
Window::~Window(){if(impl_){glfwDestroyWindow(impl_->window);delete impl_;glfwTerminate();}}
bool Window::poll(){clicked=false;wheel=0;keys.clear();glfwPollEvents();return !closing&&!glfwWindowShouldClose(impl_->window);}
void Window::present(){glfwSwapBuffers(impl_->window);}
void Window::title(const std::string& value){glfwSetWindowTitle(impl_->window,value.c_str());}
void* Window::proc(const char* name) const{return reinterpret_cast<void*>(glfwGetProcAddress(name));}
} // namespace fixture
#endif
