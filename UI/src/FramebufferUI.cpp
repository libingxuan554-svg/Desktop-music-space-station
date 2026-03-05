#include "FramebufferUI.hpp"
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <iostream>
#include <algorithm>

namespace UI {


/**
 * Implementation Logic:
 * 1. Open standard Linux Framebuffer device /dev/fb0.
 * 2. Retrieve display geometry via ioctl (FBIOGET_FSCREENINFO for fixed hardware info and FBIOGET_VSCREENINFO for variable resolution/bit depth).
 * 3. Core: Map kernel-space video memory to user-space via mmap to bypass overhead from write() system calls, ensuring real-time performance.
 */
FramebufferUI::FramebufferUI(const std::string& device) {
    fbfd = open(device.c_str(), O_RDWR);
    if (fbfd == -1) {
        throw std::runtime_error("Error: cannot open framebuffer device");
    }

    // Retrieve fixed and variable screen info to calculate memory offsets and resolution.
    if (ioctl(fbfd, FBIOGET_FSCREENINFO, &finfo) == -1 ||
        ioctl(fbfd, FBIOGET_VSCREENINFO, &vinfo) == -1) {
        close(fbfd);
        throw std::runtime_error("Error reading framebuffer information");
    }

    //Calculate total buffer size: width * height * (bytes per pixel).
    screensize = vinfo.xres * vinfo.yres * vinfo.bits_per_pixel / 8;

    // Map video memory. MAP_SHARED ensures modifications are immediately reflected on screen.
    fbp = (char*)mmap(0, screensize, PROT_READ | PROT_WRITE, MAP_SHARED, fbfd, 0);
    if ((intptr_t)fbp == -1) {
        close(fbfd);
        throw std::runtime_error("Error: failed to map framebuffer device to memory");
    }
}


/**
 * Destructor Logic:
 * Explicitly unmaps memory and closes the file descriptor to ensure safe resource 
 * release upon object destruction, adhering to C++ RAII principles.
 */
FramebufferUI::~FramebufferUI() {
    if (fbp && fbp != (char*)-1) munmap(fbp, screensize);
    if (fbfd != -1) close(fbfd);
}


/**
 * Pixel Rendering:
 * 1. High-frequency utility called by drawRect; optimized for real-time deadlines.
 * 2. Bounds checking prevents memory corruption (Segmentation Faults) by returning immediately if coordinates exceed display limits (Safety-Critical).
 * 3. Calculates the precise memory offset for the target pixel.
 * 4. Writes 32-bit color data in standard BGR format.
 */
void FramebufferUI::putPixel(int x, int y, Color color) {
	
	// Boundary safety check.
    if (x < 0 || x >= (int)vinfo.xres || y < 0 || y >= (int)vinfo.yres) return;

    // Calculate memory offset using pixel depth and line length.
    // 公式：(x + x_offset) * (bytes_per_pixel) + (y + y_offset) * line_length
    long location = (x + vinfo.xoffset) * (vinfo.bits_per_pixel / 8) +
                    (y + vinfo.yoffset) * finfo.line_length;

    // Standard 32-bit (ARGB/BGRA) format implementation.
    if (vinfo.bits_per_pixel == 32) {
        *(fbp + location) = color.b;     // Blue
        *(fbp + location + 1) = color.g; // Green
        *(fbp + location + 2) = color.r; // Red
        *(fbp + location + 3) = 0;       // Alpha/Reserved
    }
}


/**
 * Primitive Rendering:
 * Renders a filled rectangle via nested loops.
 */
void FramebufferUI::drawRect(int x, int y, int w, int h, Color color) {
    for (int i = x; i < x + w; ++i) {
        for (int j = y; j < y + h; ++j) {
            putPixel(i, j, color);
        }
    }
}


/**
 * Clear Screen:
 * Fills the entire display buffer with the specified background color.
 */
void FramebufferUI::clear(Color color) {
    drawRect(0, 0, vinfo.xres, vinfo.yres, color);
}


/**
 * 待机模式逻辑：
 * Standby Mode Logic:
 * 1. Uses std::lock_guard on uiMutex to prevent data races between hardware telemetry threads and audio callback threads.
 * 2. Linearly maps temperature values to bar length for visual feedback.
 */
void FramebufferUI::refreshStandby(float temp, const std::string& timeStr) {
    std::lock_guard<std::mutex> lock(uiMutex);
    
    // Call the renderer to decouple the graphics logic on the screen
    UIRenderer::renderTechBar(this, temp, 50, 150, vinfo.xres - 100);
}


/**
 * Music Animation Logic (Real-Time Optimized):
 * 1. Frequent execution by audio callbacks; optimized via partial redraws rather than full-screen refreshes.
 * 2. "Dirty Rectangle" approach: Clears only the previous bar area with the background color before rendering the new state based on intensity.
 * 3. Significantly reduces CPU load to maintain RT-deadlines.
 */
void FramebufferUI::refreshMusicAnimation(float intensity) {
    std::lock_guard<std::mutex> lock(uiMutex);

    UIRenderer::renderMirrorEqualizer(this, intensity, vinfo.xres/2, vinfo.yres/2, vinfo.yres/2 - 50);
}

} // namespace UI
} // namespace UI