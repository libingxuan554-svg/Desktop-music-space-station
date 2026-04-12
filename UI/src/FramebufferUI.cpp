#include "FramebufferUI.hpp"
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <fstream>
#include <stdexcept>
#include <cstring>

namespace UI {

FramebufferUI::FramebufferUI(const std::string& device) {
    fbfd = open(device.c_str(), O_RDWR);
    if (fbfd == -1) throw std::runtime_error("Cannot open framebuffer device");

    if (ioctl(fbfd, FBIOGET_FSCREENINFO, &finfo) == -1 ||
        ioctl(fbfd, FBIOGET_VSCREENINFO, &vinfo) == -1) {
        close(fbfd);
        throw std::runtime_error("Error reading framebuffer info");
    }

    screensize = vinfo.xres * vinfo.yres * vinfo.bits_per_pixel / 8;
    backBuffer.resize(screensize); 
    fbp = (char*)mmap(0, screensize, PROT_READ | PROT_WRITE, MAP_SHARED, fbfd, 0);
    
    if ((intptr_t)fbp == -1) {
        close(fbfd);
        throw std::runtime_error("Failed to map framebuffer");
    }
}

FramebufferUI::~FramebufferUI() {
    if (fbp && fbp != (char*)-1) munmap(fbp, screensize);
    if (fbfd != -1) close(fbfd);
}

void FramebufferUI::putPixel(int x, int y, Color color) {
    if (x < 0 || x >= (int)vinfo.xres || y < 0 || y >= (int)vinfo.yres) return;

    long location = (x + vinfo.xoffset) * (vinfo.bits_per_pixel / 8) +
                    (y + vinfo.yoffset) * finfo.line_length;

    if (vinfo.bits_per_pixel == 32) {
        backBuffer[location] = color.b;
        backBuffer[location + 1] = color.g;
        backBuffer[location + 2] = color.r;
        backBuffer[location + 3] = 0; 
    } else if (vinfo.bits_per_pixel == 16) {
        int r = (color.r >> 3) & 0x1F;
        int g = (color.g >> 2) & 0x3F;
        int b = (color.b >> 3) & 0x1F;
        unsigned short pixel_color = (r << 11) | (g << 5) | b;
        *((unsigned short*)(backBuffer.data() + location)) = pixel_color;
    }
}

void FramebufferUI::drawRect(int x, int y, int w, int h, Color color) {
    for (int j = y; j < y + h; ++j) {        
        for (int i = x; i < x + w; ++i) {    
            putPixel(i, j, color);
        }
    }
}

void FramebufferUI::clear(Color color) {
    drawRect(0, 0, vinfo.xres, vinfo.yres, color);
}

void FramebufferUI::flush() {
    if (fbp && fbp != (char*)-1 && !backBuffer.empty()) {
        std::memcpy(fbp, backBuffer.data(), screensize);
    }
}

void FramebufferUI::drawFullscreenBmp(const std::string& path) {
    std::ifstream f(path, std::ios::binary);
    if (!f) {
        clear({5, 10, 20});
        return;
    }

    f.seekg(54, std::ios::beg);
    
    int rowSize = vinfo.xres * 3;
    int padding = (4 - (rowSize % 4)) % 4;

    for (int y = vinfo.yres - 1; y >= 0; --y) {
        long line_offset = y * finfo.line_length;
        for (int x = 0; x < (int)vinfo.xres; ++x) {
            unsigned char bgr[3];
            f.read((char*)bgr, 3);
            
            if (vinfo.bits_per_pixel == 32) {
                long pixel_pos = line_offset + x * 4;
                backBuffer[pixel_pos] = bgr[0];     
                backBuffer[pixel_pos + 1] = bgr[1]; 
                backBuffer[pixel_pos + 2] = bgr[2]; 
                backBuffer[pixel_pos + 3] = 0;      
            } 
            else if (vinfo.bits_per_pixel == 16) {
                long pixel_pos = line_offset + x * 2;
                int r = (bgr[2] >> 3) & 0x1F;
                int g = (bgr[1] >> 2) & 0x3F;
                int b = (bgr[0] >> 3) & 0x1F;
                unsigned short pixel_color = (r << 11) | (g << 5) | b;
                *((unsigned short*)(backBuffer.data() + pixel_pos)) = pixel_color;
            }
        }
        if (padding > 0) f.seekg(padding, std::ios::cur);
    }
    f.close();
}

//  核心优化：开机预解压与内存极速拷贝 👇👇👇
void FramebufferUI::preloadBmp(const std::string& name, const std::string& path) {
    std::ifstream f(path, std::ios::binary);
    if (!f) return; // 容错处理

    // 直接分配与屏幕显存完全同等大小的缓存区
    std::vector<char> cacheBuffer(screensize, 0);

    f.seekg(54, std::ios::beg);
    int rowSize = vinfo.xres * 3;
    int padding = (4 - (rowSize % 4)) % 4;

    // 解码像素，写入专用的 cacheBuffer 中
    for (int y = vinfo.yres - 1; y >= 0; --y) {
        long line_offset = y * finfo.line_length;
        for (int x = 0; x < (int)vinfo.xres; ++x) {
            unsigned char bgr[3];
            f.read((char*)bgr, 3);

            if (vinfo.bits_per_pixel == 32) {
                long pixel_pos = line_offset + x * 4;
                cacheBuffer[pixel_pos] = bgr[0];     
                cacheBuffer[pixel_pos + 1] = bgr[1]; 
                cacheBuffer[pixel_pos + 2] = bgr[2]; 
                cacheBuffer[pixel_pos + 3] = 0;      
            } else if (vinfo.bits_per_pixel == 16) {
                long pixel_pos = line_offset + x * 2;
                int r = (bgr[2] >> 3) & 0x1F;
                int g = (bgr[1] >> 2) & 0x3F;
                int b = (bgr[0] >> 3) & 0x1F;
                unsigned short pixel_color = (r << 11) | (g << 5) | b;
                *((unsigned short*)(cacheBuffer.data() + pixel_pos)) = pixel_color;
            }
        }
        if (padding > 0) f.seekg(padding, std::ios::cur);
    }
    f.close();

    // 将解压好的显存级数据移入字典
    bmpCache[name] = std::move(cacheBuffer);
}

void FramebufferUI::drawPreloadedBmp(const std::string& name) {
    auto it = bmpCache.find(name);
    if (it != bmpCache.end()) {
        // O(N) 极速拷贝：没有任何文件 I/O，没有计算转换，瞬间完成渲染
        std::memcpy(backBuffer.data(), it->second.data(), screensize);
    } else {
        clear({5, 10, 20}); // 保底策略
    }
}

} // namespace UI
