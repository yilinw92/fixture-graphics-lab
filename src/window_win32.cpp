#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <windowsx.h>
#include <GL/gl.h>
#include "fixture/window.hpp"
#include <stdexcept>

namespace fixture {
struct Window::Impl {
    HWND window{}; HDC dc{}; HGLRC context{};
    ~Impl(){
        if(context){wglMakeCurrent(nullptr,nullptr);wglDeleteContext(context);}
        if(dc && window) ReleaseDC(window,dc);
        if(window) DestroyWindow(window);
    }
};
static LRESULT CALLBACK callback(HWND hwnd,UINT msg,WPARAM wp,LPARAM lp) {
    auto* state=reinterpret_cast<Window*>(GetWindowLongPtrW(hwnd,GWLP_USERDATA));
    if(msg==WM_NCCREATE){state=static_cast<Window*>(reinterpret_cast<CREATESTRUCTW*>(lp)->lpCreateParams);SetWindowLongPtrW(hwnd,GWLP_USERDATA,reinterpret_cast<LONG_PTR>(state));}
    if(state) switch(msg){
        case WM_CLOSE: state->closing=true;return 0;
        case WM_SIZE: state->width=LOWORD(lp)>0?LOWORD(lp):1;state->height=HIWORD(lp)>0?HIWORD(lp):1;return 0;
        case WM_MOUSEMOVE: state->mouse_x=GET_X_LPARAM(lp);state->mouse_y=GET_Y_LPARAM(lp);return 0;
        case WM_LBUTTONDOWN: state->clicked=true;state->mouse_x=GET_X_LPARAM(lp);state->mouse_y=GET_Y_LPARAM(lp);return 0;
        case WM_MOUSEWHEEL: state->wheel+=GET_WHEEL_DELTA_WPARAM(wp)/120.0;return 0;
        case WM_KEYDOWN: if(wp==VK_ESCAPE)state->closing=true;else state->keys.push_back(int(wp));return 0;
    }
    return DefWindowProcW(hwnd,msg,wp,lp);
}
Window::Window(bool hidden) {
    impl_=new Impl;
    try {
        WNDCLASSW wc{};wc.style=CS_OWNDC;wc.lpfnWndProc=callback;wc.hInstance=GetModuleHandleW(nullptr);wc.lpszClassName=L"FixtureGraphicsLab";
        wc.hCursor=LoadCursor(nullptr,IDC_ARROW);
        if(!RegisterClassW(&wc) && GetLastError()!=ERROR_CLASS_ALREADY_EXISTS)throw std::runtime_error("RegisterClass failed");
        RECT rect{0,0,width,height};AdjustWindowRect(&rect,WS_OVERLAPPEDWINDOW,FALSE);
        impl_->window=CreateWindowExW(0,wc.lpszClassName,L"Fixture Graphics Lab",WS_OVERLAPPEDWINDOW,CW_USEDEFAULT,CW_USEDEFAULT,rect.right-rect.left,rect.bottom-rect.top,nullptr,nullptr,wc.hInstance,this);
        if(!impl_->window)throw std::runtime_error("CreateWindow failed");
        impl_->dc=GetDC(impl_->window);
        PIXELFORMATDESCRIPTOR pf{};pf.nSize=sizeof(pf);pf.nVersion=1;pf.dwFlags=PFD_DRAW_TO_WINDOW|PFD_SUPPORT_OPENGL|PFD_DOUBLEBUFFER;pf.iPixelType=PFD_TYPE_RGBA;pf.cColorBits=32;pf.cDepthBits=24;
        const int format=ChoosePixelFormat(impl_->dc,&pf);
        if(!format || !SetPixelFormat(impl_->dc,format,&pf))throw std::runtime_error("OpenGL pixel format unavailable");
        impl_->context=wglCreateContext(impl_->dc);
        if(!impl_->context || !wglMakeCurrent(impl_->dc,impl_->context))throw std::runtime_error("WGL bootstrap context failed");
        using CreateContext=HGLRC(WINAPI*)(HDC,HGLRC,const int*);
        auto create=reinterpret_cast<CreateContext>(reinterpret_cast<void*>(wglGetProcAddress("wglCreateContextAttribsARB")));
        if(!create)throw std::runtime_error("OpenGL 3.3 requires a GPU driver; use Linux Mesa/Xvfb for headless capture");
        const int attributes[]={0x2091,3,0x2092,3,0x9126,1,0};
        const HGLRC modern=create(impl_->dc,nullptr,attributes);
        if(!modern)throw std::runtime_error("OpenGL 3.3 core context creation failed");
        wglMakeCurrent(nullptr,nullptr);wglDeleteContext(impl_->context);impl_->context=modern;
        if(!wglMakeCurrent(impl_->dc,modern))throw std::runtime_error("Cannot activate OpenGL 3.3");
        if(!hidden)ShowWindow(impl_->window,SW_SHOW);
    } catch(...) {delete impl_;impl_=nullptr;throw;}
}
Window::~Window(){delete impl_;}
bool Window::poll(){
    clicked=false;wheel=0;keys.clear();MSG msg;
    while(PeekMessageW(&msg,nullptr,0,0,PM_REMOVE)){TranslateMessage(&msg);DispatchMessageW(&msg);}
    return !closing;
}
void Window::present(){SwapBuffers(impl_->dc);}
void Window::title(const std::string& value){SetWindowTextA(impl_->window,value.c_str());}
void* Window::proc(const char* name) const {
    const auto p=wglGetProcAddress(name);
    if(!p || p==reinterpret_cast<PROC>(1) || p==reinterpret_cast<PROC>(2) || p==reinterpret_cast<PROC>(3) || p==reinterpret_cast<PROC>(-1))
        return reinterpret_cast<void*>(GetProcAddress(GetModuleHandleA("opengl32.dll"),name));
    return reinterpret_cast<void*>(p);
}
} // namespace fixture
#endif
