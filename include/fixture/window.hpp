#pragma once
#include <string>
#include <vector>

namespace fixture {
class Window {
public:
    explicit Window(bool hidden=false);
    ~Window();
    Window(const Window&)=delete;
    Window& operator=(const Window&)=delete;
    bool poll();
    void present();
    void title(const std::string& value);
    void* proc(const char* name) const;
    int width{1000},height{700},mouse_x{},mouse_y{};
    bool clicked{},closing{};
    double wheel{};
    std::vector<int> keys;
private:
    struct Impl;
    Impl* impl_{};
};
} // namespace fixture
