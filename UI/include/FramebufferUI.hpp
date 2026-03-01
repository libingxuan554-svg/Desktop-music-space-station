 #ifndef FRAMEBUFFER_UI_HPP
#define FRAMEBUFFER_UI_HPP

#include <linux/fb.h>
#include <stdint.h>
#include <vector>
#include <string>
#include <mutex>

/**
 * @namespace UI
 * @brief Contains classes and utilities for low-level graphical rendering.
 */
namespace UI {


/**
 * @struct Color
 * @brief Standard RGB color structure.
 */
struct Color {
    uint8_t r; ///< Red   (0-255)
    uint8_t g; ///< Green (0-255)
    uint8_t b; ///< Blue  (0-255)
};


/**
 * @class FramebufferUI
 * @brief Handles direct-to-memory graphical rendering via the Linux Framebuffer device.
 * * Adheres to real-time embedded programming standards:
 * 1. 1. Bypasses high-level frameworks (e.g., Qt) using mmap for minimal rendering latency.
 * 2. Thread-safe design for concurrent updates from hardware/audio threads.
 * 3. Zero dynamic memory allocation post-initialization (AUTOSAR C++14 compliant).
 */
class FramebufferUI {
public:


    /**
     * @brief Constructor: Initializes the Framebuffer and maps video memory.
     * @param device Device path, defaults to "/dev/fb0".
     * @throw std::runtime_error On device access or memory mapping failure.
     */
    FramebufferUI(const std::string& device = "/dev/fb0");


    /**
     * @brief Destructor: Unmaps video memory and closes the file descriptor.
     */
    ~FramebufferUI();


    /**
     * @brief Standby Refresh: Updates telemetry (temperature and time) on screen.
     * @note Thread-safe. Performs a full screen clear and background redraw.
     * @param temp Current temperature in Celsius.
     * @param timeStr Formatted timestamp.
     */
    void refreshStandby(float temp, const std::string& timeStr);


    /**
     * @brief Animation Refresh: Updates visuals based on audio rhythm intensity.
     * @note High-frequency call; optimized via partial screen updates.
     * @param intensity Rhythm intensity normalized to [0.0, 1.0].
     */
    void refreshMusicAnimation(float intensity); 


    /**
     * @brief Fills the entire buffer with a specific color.
     * @param color Target fill color.
     */
    void clear(Color color = {0, 0, 0});

private:

    /**
     * @brief Renders a filled rectangle at specified coordinates.
     * @param x X-coordinate (top-left).
     * @param y Y-coordinate (top-left).
     * @param w Width in pixels.
     * @param h Height in pixels.
     * @param color Fill color.
     */
    void drawRect(int x, int y, int w, int h, Color color);


    /**
     * @brief Renders a single pixel.
     * @note Includes boundary checks to prevent memory corruption.
     * @param x Pixel X-coordinate.
     * @param y Pixel Y-coordinate.
     * @param color Pixel color.
     */
    void putPixel(int x, int y, Color color);

    // Framebuffer Resources
    int fbfd;                           ///< File descriptor.
    struct fb_var_screeninfo vinfo;     ///< Variable screen info (resolution).
    struct fb_fix_screeninfo finfo;     ///< Fixed screen info (memory metadata).
    long screensize;                    ///< Total mapped size in bytes.
    char* fbp;                          ///< Mapped memory pointer.


    /**
     * @brief Mutex protecting the video buffer from race conditions and tearing.
     */
    std::mutex uiMutex; 
};

} // namespace UI

#endif // FRAMEBUFFER_UI_HPP
