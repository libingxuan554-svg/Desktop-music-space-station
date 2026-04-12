#ifndef FRAMEBUFFER_UI_HPP
#define FRAMEBUFFER_UI_HPP

#include <linux/fb.h>
#include <stdint.h>
#include <string>
#include <mutex>
#include <vector>
#include <map>

namespace UI {

struct Color {
    uint8_t r;
    uint8_t g;
    uint8_t b;
};

class FramebufferUI {

public:
    FramebufferUI(const std::string& device = "/dev/fb0");
    ~FramebufferUI();

    void drawRect(int x, int y, int w, int h, Color color);
    void clear(Color color = {0, 0, 0});
    void flush(); // 双缓冲推送接口

    void drawFullscreenBmp(const std::string& path);
    //内存预加载与零延迟渲染接口
    void preloadBmp(const std::string& name, const std::string& path);
    void drawPreloadedBmp(const std::string& name);

    int getWidth() const { return vinfo.xres; }
    int getHeight() const { return vinfo.yres; }

    void lock() { uiMutex.lock(); }
    void unlock() { uiMutex.unlock(); }

private:
    void putPixel(int x, int y, Color color);

    int fbfd;
    struct fb_var_screeninfo vinfo;
    struct fb_fix_screeninfo finfo;
    long screensize;
    char* fbp;
    std::vector<char> backBuffer; // 内存双缓冲
    std::mutex uiMutex;
    std::map<std::string, std::vector<char>> bmpCache; // 预解析的纯像素数据缓存

};

} // namespace UI
#endif
