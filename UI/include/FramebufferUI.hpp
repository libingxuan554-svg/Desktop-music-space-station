#ifndef FRAMEBUFFER_UI_HPP
#define FRAMEBUFFER_UI_HPP

#include <linux/fb.h> //Include Linux framebuffer device structures
#include <stdint.h>   //Include standard integer definitions
#include <string>     //Include string class
#include <mutex>      //Include mutex for thread safety
#include <vector>     //Include dynamic array container
#include <map>        //Include key-value map container

// Define UI namespace
namespace UI {

/**
 * @struct Color
 * @brief Defines the RGB color structure.
 * @note Architecture / SOLID
 * Follows SRP: only responsible for color representation.
 */
struct Color {
    uint8_t r;  //Red component
    uint8_t g;  //Green component
    uint8_t b;  //Blue component
};

/**
 * @class FramebufferUI
 * @brief Framebuffer graphics driver class for memory mapping and pixel operations.
 * @note Architecture / SOLID:
 * Follows SRP: only responsible for hardware abstraction.
 * RAII: map in constructor, unmap in destructor.
 */
class FramebufferUI {

public:

/**
 * @brief Constructor: initialize device and establish memory mapping.
 * @param[in] device: Device path.
 * @note Real-Time Constraints:
 * Determinism: mmap syscall performed only at startup.
 */
    FramebufferUI(const std::string& device = "/dev/fb0");  //Map framebuffer into user space

/**
 * @brief Destructor: unmap memory and close device.
 */
    ~FramebufferUI();

/**
 * @brief Draw a filled rectangle in the back buffer.
 * @param[in] x  X coordinate
 * @param[in] y  Y coordinate
 * @param[in] w  Width
 * @param[in] h  Height
 * @param[in] color 
 * @note Real-Time Constraints:
 * Memory op: pure memory write, no disk I/O blocking.
 */
    void drawRect(int x, int y, int w, int h, Color color);

/**
 * @brief  Clear the screen with a specified color.
 * @param[in] color  Background color.
 */
    void clear(Color color = {0, 0, 0});

/**
 * @brief  Push back buffer content to physical VRAM.
 * @note Real-Time Constraints:
 * Efficiency: use memcpy for efficient VRAM transfer.
 */                              
    void flush(); 
/**
 * @brief Draw a full-screen BMP image.
 * @param[in] path:  BMP file path.
 */
    void drawFullscreenBmp(const std::string& path);

/**
 * @brief  Preload BMP images into memory cache.
 * @param[in] name  Cache identifier name.
 * @param[in] path  Resource file path.
 */
    void preloadBmp(const std::string& name, const std::string& path);

/**
 * @brief  Render using preloaded data.
 * @param[in] name  Cache identifier name.
 * @note Real-Time Constraints:
 * Zero disk I/O: memory copy only, ensures refresh rate.
 */
    void drawPreloadedBmp(const std::string& name);

    int getWidth() const { return vinfo.xres; }  //Return horizontal resolution
    int getHeight() const { return vinfo.yres; } //Return vertical resolution

/**
 * @brief  Lock UI for thread-safe operations.
 * @note Real-Time Constraints:
 * // REAL-TIME COMPLIANCE:
 * // Use mutex to protect the shared backBuffer
 * // Prevent screen tearing from simultaneous memory modification
 */
    void lock() { uiMutex.lock(); }

/**
 * @brief  Unlock the UI.
 */
    void unlock() { uiMutex.unlock(); }

private:

/**
 * @brief  Plot a pixel in memory buffer.
 * @param[in] x  X coordinate
 * @param[in] y  Y coordinate
 * @param[in] color  Color format.
 */
    void putPixel(int x, int y, Color color);

    int fbfd;  //Framebuffer device file descriptor
    struct fb_var_screeninfo vinfo;  //Store variable screen information
    struct fb_fix_screeninfo finfo;  //Store fixed screen information
    long screensize;  //Calculated total framebuffer size
    char* fbp;  //Starting mapped address of hardware VRAM
    std::vector<char> backBuffer; // Software dual-buffer layer to prevent flickering
    std::mutex uiMutex;  //Mutex ensuring multi-threaded rendering safety
    std::map<std::string, std::vector<char>> bmpCache; //High-speed dictionary storing pre-decompressed pixels

};

} // namespace UI
#endif
