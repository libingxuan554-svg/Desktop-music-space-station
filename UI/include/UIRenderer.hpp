#ifndef UI_RENDERER_HPP
#define UI_RENDERER_HPP

#include <vector>     // Include dynamic array container
#include <string>     // Include string support
#include <functional> // Include function wrapper for button callback support
#include "FramebufferUI.hpp" // Include low-level framebuffer driver interface

// Define UI namespace
namespace UI {

/**
 * @struct UIButton
 * @brief Defines the button structure and callback wrapper for the interactive interface
 * @note [Architecture / SOLID]:
 * Follows SRP: acts only as a carrier for interaction areas and states, containing no specific rendering logic.
 */
struct UIButton {
    int x, y, w, h; //Physical X, Y coordinates and width/height dimensions of the button
    std::string id; // Unique identifier or display text of the button
    Color color;    // Base color configuration of the button
    std::function<void()> action; // Callback function executed when the button is triggered by touch
};

/**
 * @class UIRenderer
 * @brief High-level graphics rendering engine, providing drawing capabilities for various customized UI components.
 * @note [Architecture / SOLID]:
 * Strategy pattern & DIP: completely isolates business logic, focusing on pure pixel algorithms by passing FramebufferUI pointers for low-level operations.
 * Pure static stateless design: avoids object instantiation overhead, improving real-time rendering performance.
 */
class UIRenderer {

public:

/**
 * @brief Draw a tech-style single energy bar
 * @param[in] ui  Pointer to low-level framebuffer driver
 * @param[in] value  Energy ratio (0.0-1.0)
 * @param[in] x X  X coordinate
 * @param[in] y Y  Y coordinate
 * @param[in] maxW  Maximum pixel width
 */
    static void renderTechBar(FramebufferUI* ui, float value, int x, int y, int maxW);

/**
 * @brief Draw a mirrored single-band equalizer
 * @param[in] ui Pointer to low-level driver
 * @param[in] intensity  Audio intensity
 * @param[in] cx Center X coordinate
 * @param[in] cy  Center Y coordinate
 * @param[in] maxH  Maximum one-sided height
 */
    static void renderMirrorEqualizer(FramebufferUI* ui, float intensity, int cx, int cy, int maxH);
 
/**
 * @brief Render the jumping multi-bar spectrum at the bottom of the player page
 * @note [Real-Time Constraints]:
 * // [REAL-TIME COMPLIANCE]:
 * // Pure memory fixed-length loop drawing, no complex floating-point square root operations, ensuring rendering completes within 33ms budget, meeting 30FPS real-time constraints.
 */
    static void renderMultiBarEQ(FramebufferUI* ui, float intensity, int cx, int cy, int maxH);

//============ PLAYER ============

/**
 * @brief Render the cyber-minimalist wireframe button exclusive to the player
 * @param[in] btn  Button data structure
 * @param[in] isHighlighted  Whether it is in an active highlighted state
 */
    static void renderButton(FramebufferUI* ui, const UIButton& btn, bool isHighlighted = false); 

/**
 * @brief  Draw dot-matrix text using a custom 8x8 pixel font library
 * @param[in] text  String to be rendered
 * @param[in] scale  Pixel scaling factor
 * @param[in] clipX1  X-axis left clipping boundary
 * @param[in] clipX2  X-axis right clipping boundary
 */
    static void drawText(FramebufferUI* ui, const std::string& text, int x, int y, Color color, int scale = 2, int clipX1 = 0, int clipX2 = 1024); 

/**
 * @brief  Draw playback control bar with progress fill
 * @param[in] progress  Playback progress (0.0-1.0)
 */
    static void renderProgressBar(FramebufferUI* ui, float progress, int x, int y, int w, int h); 

// ============ MUSIC_LIST ============

/**
 * @brief  Render the chamfered cyber frame exclusive to the list, supporting dynamic Y-axis clipping
 * @param[in] clipY1  Top visible boundary of the list area
 * @param[in] clipY2  Bottom visible boundary of the list area
 * @note [Real-Time Constraints]:
 * // [REAL-TIME COMPLIANCE]:
 * // Using early-out boundary algorithms to completely skip pixel calculations outside the viewport, greatly reducing CPU usage during scrolling.
 */
    static void renderListButton(FramebufferUI* ui, int x, int y, int w, int h, bool isSelected, int clipY1, int clipY2);

/**
 * @brief  Draw list text with 2D boundary clipping
 * @param[in] clipY1  Y-axis top clipping boundary
 * @param[in] clipY2  Y-axis bottom clipping boundary
 */
    static void drawListText(FramebufferUI* ui, const std::string& text, int x, int y, Color color, int scale, int clipX1, int clipX2, int clipY1, int clipY2); 

// ============ STANDBY ============

/**
 * @brief Render the gradient segmented memory usage bar exclusive to the standby page
 * @param[in] percent  Current memory usage percentage
 */
    static void renderStandbyMemoryBar(FramebufferUI* ui, int x, int y, int w, int h, int percent); 

/**
 * @brief Generate pixel-art weather icons using pure code
 * @param[in] weatherCode  Weather code parsed from wttr.in
 */
    static void renderWeatherIcon(FramebufferUI* ui, int x, int y, int weatherCode); 

/**
 * @brief Render the node breathing light effect on the topology background board
 * @param[in] phase  Current time phase, driving the sine wave breathing frequency
 */
    static void renderTopologyBreathingLights(FramebufferUI* ui, int cx, int cy, float phase); 
};

}
#endif
