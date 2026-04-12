#include "UIRenderer.hpp" // Include UI renderer header
#include <algorithm>      // Include standard algorithm library
#include <cmath>          // Include math library for calculating trigonometric functions

namespace UI {

/**
 * @brief Statically embedded 8x8 pixel dot-matrix font library
 * @note [Architecture / SOLID]:
 * Pure static read-only data, avoiding runtime memory allocation and file I/O read overhead.
 */
static const unsigned char font8x8[96][8] = {
    {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},{0x18,0x3C,0x3C,0x18,0x18,0x00,0x18,0x00},{0x6C,0x6C,0x6C,0x00,0x00,0x00,0x00,0x00},{0x6C,0x6C,0xFE,0x6C,0xFE,0x6C,0x6C,0x00},
    {0x18,0x7E,0xC0,0x7C,0x06,0xFC,0x18,0x00},{0x00,0xC6,0xCC,0x18,0x30,0x66,0xC6,0x00},{0x38,0x6C,0x6C,0x38,0x6D,0x66,0x3B,0x00},{0x18,0x18,0x30,0x00,0x00,0x00,0x00,0x00},
    {0x0C,0x18,0x30,0x30,0x30,0x18,0x0C,0x00},{0x30,0x18,0x0C,0x0C,0x0C,0x18,0x30,0x00},{0x00,0x66,0x3C,0xFF,0x3C,0x66,0x00,0x00},{0x00,0x18,0x18,0x7E,0x18,0x18,0x00,0x00},
    {0x00,0x00,0x00,0x00,0x00,0x18,0x18,0x30},{0x00,0x00,0x00,0x7E,0x00,0x00,0x00,0x00},{0x00,0x00,0x00,0x00,0x00,0x18,0x18,0x00},{0x00,0x06,0x0C,0x18,0x30,0x60,0x00,0x00},
    {0x3C,0x66,0x6E,0x76,0x66,0x66,0x3C,0x00},{0x18,0x38,0x18,0x18,0x18,0x18,0x7E,0x00},{0x3C,0x66,0x06,0x1C,0x30,0x60,0x7E,0x00},{0x3C,0x66,0x06,0x1C,0x06,0x66,0x3C,0x00},
    {0x0C,0x1C,0x3C,0x6C,0x7E,0x0C,0x0C,0x00},{0x7E,0x60,0x7C,0x06,0x06,0x66,0x3C,0x00},{0x3C,0x66,0x60,0x7C,0x66,0x66,0x3C,0x00},{0x7E,0x06,0x0C,0x18,0x30,0x30,0x30,0x00},
    {0x3C,0x66,0x66,0x3C,0x66,0x66,0x3C,0x00},{0x3C,0x66,0x66,0x3E,0x06,0x66,0x3C,0x00},{0x00,0x18,0x18,0x00,0x00,0x18,0x18,0x00},{0x00,0x18,0x18,0x00,0x00,0x18,0x18,0x30},
    {0x06,0x0C,0x18,0x30,0x18,0x0C,0x06,0x00},{0x00,0x00,0x7E,0x00,0x7E,0x00,0x00,0x00},{0x60,0x30,0x18,0x0C,0x18,0x30,0x60,0x00},{0x3C,0x66,0x06,0x0C,0x18,0x00,0x18,0x00},
    {0x3C,0x66,0x6E,0x6E,0x60,0x66,0x3C,0x00},{0x18,0x3C,0x66,0x66,0x7E,0x66,0x66,0x00},{0x7C,0x66,0x66,0x7C,0x66,0x66,0x7C,0x00},{0x3C,0x66,0x60,0x60,0x60,0x66,0x3C,0x00},
    {0x78,0x6C,0x66,0x66,0x66,0x6C,0x78,0x00},{0x7E,0x60,0x60,0x7C,0x60,0x60,0x7E,0x00},{0x7E,0x60,0x60,0x7C,0x60,0x60,0x60,0x00},{0x3C,0x66,0x60,0x6E,0x66,0x66,0x3E,0x00},
    {0x66,0x66,0x66,0x7E,0x66,0x66,0x66,0x00},{0x3C,0x18,0x18,0x18,0x18,0x18,0x3C,0x00},{0x06,0x06,0x06,0x06,0x06,0x66,0x3C,0x00},{0x66,0x6C,0x78,0x70,0x78,0x6C,0x66,0x00},
    {0x60,0x60,0x60,0x60,0x60,0x60,0x7E,0x00},{0x63,0x77,0x7F,0x6B,0x63,0x63,0x63,0x00},{0x66,0x76,0x7E,0x7E,0x6E,0x66,0x66,0x00},{0x3C,0x66,0x66,0x66,0x66,0x66,0x3C,0x00},
    {0x7C,0x66,0x66,0x7C,0x60,0x60,0x60,0x00},{0x3C,0x66,0x66,0x66,0x6A,0x6C,0x36,0x00},{0x7C,0x66,0x66,0x7C,0x6C,0x66,0x66,0x00},{0x3C,0x66,0x60,0x3C,0x06,0x66,0x3C,0x00},
    {0x7E,0x18,0x18,0x18,0x18,0x18,0x18,0x00},{0x66,0x66,0x66,0x66,0x66,0x66,0x3C,0x00},{0x66,0x66,0x66,0x66,0x66,0x3C,0x18,0x00},{0x63,0x63,0x63,0x6B,0x7F,0x77,0x63,0x00},
    {0x66,0x66,0x3C,0x18,0x3C,0x66,0x66,0x00},{0x66,0x66,0x66,0x3C,0x18,0x18,0x18,0x00},{0x7E,0x06,0x0C,0x18,0x30,0x60,0x7E,0x00},{0x3C,0x30,0x30,0x30,0x30,0x30,0x3C,0x00},
    {0x00,0x60,0x30,0x18,0x0C,0x06,0x00,0x00},{0x3C,0x0C,0x0C,0x0C,0x0C,0x0C,0x3C,0x00},{0x18,0x3C,0x66,0x00,0x00,0x00,0x00,0x00},{0x00,0x00,0x00,0x00,0x00,0x00,0xFF,0x00},
    {0x30,0x18,0x00,0x00,0x00,0x00,0x00,0x00},{0x00,0x00,0x3C,0x06,0x3E,0x66,0x3E,0x00},{0x60,0x60,0x7C,0x66,0x66,0x66,0x7C,0x00},{0x00,0x00,0x3C,0x60,0x60,0x60,0x3C,0x00},
    {0x06,0x06,0x3E,0x66,0x66,0x66,0x3E,0x00},{0x00,0x00,0x3C,0x66,0x7E,0x60,0x3C,0x00},{0x1C,0x30,0x7C,0x30,0x30,0x30,0x30,0x00},{0x00,0x00,0x3E,0x66,0x66,0x3E,0x06,0x3C},
    {0x60,0x60,0x7C,0x66,0x66,0x66,0x66,0x00},{0x18,0x00,0x38,0x18,0x18,0x18,0x3C,0x00},{0x0C,0x00,0x1C,0x0C,0x0C,0x0C,0x0C,0x38},{0x60,0x60,0x66,0x6C,0x78,0x6C,0x66,0x00},
    {0x38,0x18,0x18,0x18,0x18,0x18,0x3C,0x00},{0x00,0x00,0x66,0x7F,0x7F,0x6B,0x63,0x00},{0x00,0x00,0x7C,0x66,0x66,0x66,0x66,0x00},{0x00,0x00,0x3C,0x66,0x66,0x66,0x3C,0x00},
    {0x00,0x00,0x7C,0x66,0x66,0x7C,0x60,0x60},{0x00,0x00,0x3E,0x66,0x66,0x3E,0x06,0x06},{0x00,0x00,0x7C,0x60,0x60,0x60,0x60,0x00},{0x00,0x00,0x3E,0x60,0x3C,0x06,0x3C,0x00},
    {0x30,0x78,0x30,0x30,0x30,0x30,0x1C,0x00},{0x00,0x00,0x66,0x66,0x66,0x66,0x3E,0x00},{0x00,0x00,0x66,0x66,0x66,0x3C,0x18,0x00},{0x00,0x00,0x63,0x6B,0x7F,0x3E,0x36,0x00},
    {0x00,0x00,0x66,0x3C,0x18,0x3C,0x66,0x00},{0x00,0x00,0x66,0x66,0x66,0x3E,0x06,0x3C},{0x00,0x00,0x7E,0x0C,0x18,0x30,0x7E,0x00},{0x0E,0x18,0x18,0x70,0x18,0x18,0x0E,0x00},
    {0x18,0x18,0x18,0x00,0x18,0x18,0x18,0x00},{0x70,0x18,0x18,0x0E,0x18,0x18,0x70,0x00},{0x3B,0x6E,0x00,0x00,0x00,0x00,0x00,0x00},{0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}
};

/**
 * @brief Internal static helper: Rectangle drawing with Y-axis boundary clipping protection
 * @param[in] ui  Pointer to low-level driver
 * @param[in] x  X coordinate
 * @param[in] y  Y coordinate
 * @param[in] w  Width
 * @param[in] h  Height
 * @param[in] color  Color
 * @param[in] clipY1  Y-axis top clipping bound
 * @param[in] clipY2  Y-axis bottom clipping bound
 * @note [Real-Time Constraints]:
 * // [REAL-TIME COMPLIANCE]:
 * // High-performance viewport culling algorithm.
 * // Only calculates pixels within the safe visible area, greatly reducing pure CPU calculation time during long list scrolling.
 */
// Coordinate pixel truncator with Y-axis limits with high performance
static void drawClippedRect(FramebufferUI* ui, int x, int y, int w, int h, Color color, int clipY1, int clipY2) {
    int startY = std::max(y, clipY1);                         // Get safe top start point
    int endY = std::min(y + h, clipY2);                       // Get safe bottom end point
    if (startY < endY && w > 0) {                             // If graphic still has height and width after truncation
        ui->drawRect(x, startY, w, endY - startY, color);     // Call low-level to execute safe drawing
    }
}

/**
 * @brief Draw a tech-style single energy bar
 */
void UIRenderer::renderTechBar(FramebufferUI* ui, float value, int x, int y, int maxW) {
    int barLen = std::min(static_cast<int>(value * (maxW / 50.0f)), maxW); 
    //Calculate mapped width limiting maximum value
    ui->drawRect(x, y, barLen, 40, {255, 100, 0}); //Draw orange long bar
}

/**
 * @brief Draw a single-sided mirrored equalizer pillar
 */
void UIRenderer::renderMirrorEqualizer(FramebufferUI* ui, float intensity, int cx, int cy, int maxH) {
    int h = static_cast<int>(intensity * maxH);                 //Calculate pixel height based on spectrum energy
    ui->drawRect(cx - 60, cy - h, 40, h * 2, {0, 255, 150});    // Draw symmetrically expanding from the center point
}

// ============== Text drawing magic ==============
/**
 * @brief High-performance dot-matrix text rendering
 */
void UIRenderer::drawText(FramebufferUI* ui, const std::string& text, int x, int y, Color color, int scale, int clipX1, int clipX2) {
    for (size_t i = 0; i < text.length(); ++i) {         // Iterate through target string
        unsigned char c = text[i];                       // Extract single char
        if (c < 32 || c > 127) c = 63;                   // Filter non-standard ASCII, map to question mark
        const unsigned char* glyph = font8x8[c - 32];    // Extract glyph array from font data dictionary
        for (int col = 0; col < 8; ++col) {              // Scan columns
            int pixelX = x + (i * 8 + col) * scale;      // Calculate absolute column X coordinate
            if (pixelX < clipX1 || pixelX + scale > clipX2) continue; // Do not render if out of X-axis viewport
            for (int row = 0; row < 8; ++row) {                       // Scan rows
                if (glyph[row] & (1 << (7 - col))) {                  // Use bitwise operation to match font active pixels
                    ui->drawRect(pixelX, y + row * scale, scale, scale, color); // Scale logical pixel to physical screen
                }
            }
        }
    }
}

// ============== PLAYER exclusive minimalist 1px border for buttons ==============
/**
 * @brief Draw minimalist 1px wireframe button exclusive to the player
 */
void UIRenderer::renderButton(FramebufferUI* ui, const UIButton& btn, bool isHighlighted) {
    if (btn.y + btn.h < 0 || btn.y > ui->getHeight()) return; //  Screen-out culling
    Color frameColor = isHighlighted ? Color{255, 240, 150} : Color{0, 200, 255}; //Selected beige state, unselected cyber cyan
    ui->drawRect(btn.x, btn.y, btn.w, 1, frameColor);             // Draw top border line
    ui->drawRect(btn.x, btn.y + btn.h - 1, btn.w, 1, frameColor); // Draw bottom border line
    ui->drawRect(btn.x, btn.y, 1, btn.h, frameColor);             // Draw left border line
    ui->drawRect(btn.x + btn.w - 1, btn.y, 1, btn.h, frameColor); // Draw right border line
}

// ============== PLAYER jumping spectrum and progress bar ==============
/**
 * @brief Draw playback progress bar
 */
void UIRenderer::renderProgressBar(FramebufferUI* ui, float progress, int x, int y, int w, int h) {
    ui->drawRect(x, y + h/2, w, 1, {0, 60, 80});   // Draw dark base slot line
    int filledW = static_cast<int>(progress * w);  // Convert percentage to physical pixel length
    if (filledW > 0) {                             // Render progress cover
        ui->drawRect(x, y + h/2, filledW, 2, {0, 200, 255});       // Bright cyan cover progress
        ui->drawRect(x + filledW - 4, y, 8, h, {255, 245, 180});   // Bright flashing cursor at the front of the progress bar
    }
}

/**
 * @brief Draw multi-band jumping spectrum analyzer
 * @note [Real-Time Constraints]:
 * // [REAL-TIME COMPLIANCE]:
 * // Uses internal static arrays to maintain inter-frame state, avoiding memory fragmentation and time overhead caused by per-frame new/delete.
 */
void UIRenderer::renderMultiBarEQ(FramebufferUI* ui, float intensity, int cx, int cy, int maxH) {
    const int numBars = 21;    // Define total number of spectrum bars
    const int barW = 16;       // Define single bar width
    const int spacing = 10;     // Define gap between bars
    const int totalW = numBars * barW + (numBars - 1) * spacing; // Calculate total width span
    const int startX = cx - totalW / 2;     // Calculate centered start X
    static float barHeights[numBars] = {0}; // Static persistence buffer: keep height from previous frame
    static float personality[numBars] = {0.7f, 1.4f, 0.6f, 1.1f, 1.8f, 0.9f, 1.3f, 0.5f, 1.6f, 1.0f, 1.9f, 0.8f, 1.4f, 0.7f, 1.2f, 0.6f, 1.7f, 0.9f, 1.5f, 0.5f, 1.1f}; // 为每根柱子赋予差异化的振幅性格权重 | Give differentiated amplitude personality weights to each bar
    static float noiseFrame = 0.0f; // Perlin noise generator timeline parameter
    noiseFrame += 0.15f;         // Push time flow
    if (intensity < 0.02f) {     // Under extremely low volume, bars accelerate falling to zero
        for (int i = 0; i < numBars; ++i) {
            barHeights[i] *= 0.82f;             // Decay at 0.82 rate
            int h = (int)barHeights[i] + 5;     // Maintain minimum 5 pixel height base
            ui->drawRect(startX + i * (barW + spacing), cy - h, barW, h, {0, 60, 80}); // Draw dark idle state
        }
        return;   // exit to save computation
    }
    for (int i = 0; i < numBars; ++i) {         // Music playing state height mapping
        float noise = std::abs(std::sin(noiseFrame + i)) * 0.05f;     // Introduce artificial fluctuation noise combined with trig function
        float targetH = (intensity + noise) * maxH * personality[i];  // Calculate target height based on overall energy + noise + personality trait
        if (targetH > barHeights[i]) barHeights[i] = targetH;         // If impact is stronger, reach highest point directly with no delay (Attack)
        else barHeights[i] -= (barHeights[i] - targetH) * 0.25f;      // If energy decays, fall slowly and smoothly with 0.25 damping (Release/Decay)
        int h = static_cast<int>(barHeights[i]);     // Float to int conversion
        h = std::clamp(h, 5, maxH);                  // Clamp to safe interval
        ui->drawRect(startX + i * (barW + spacing), cy - h, barW, h, {0, 255, 220});         // Draw main glowing bar
        ui->drawRect(startX + i * (barW + spacing), cy - h - 5, barW, 2, {200, 255, 255});   // Draw floating bright white cursor cap on top
    }
}

//============== MUSIC_LIST exclusive cyber chamfered box with top/bottom mask ==============
/**
 * @brief Draw cyber chamfered frame exclusive to music list
 */
void UIRenderer::renderListButton(FramebufferUI* ui, int x, int y, int w, int h, bool isSelected, int clipY1, int clipY2) {
    //Roughly filter completely invisible frames to save loops
    if (y + h < clipY1 || y > clipY2) return;
    Color cyan = {0, 255, 255}; // Frame fixed cyan
    int c = 15;                 //Define chamfer slope physical pixel size

    if (isSelected) {             //If in selected highlighted state
        for(int i=0; i<c; ++i) {  //Use mathematical layering algorithm to fill solid face of chamfered area
            drawClippedRect(ui, x + c - i, y + i, w - 2*c + 2*i, 1, cyan, clipY1, clipY2); 
            drawClippedRect(ui, x + c - i, y + h - 1 - i, w - 2*c + 2*i, 1, cyan, clipY1, clipY2); 
        }
        drawClippedRect(ui, x, y + c, w, h - 2*c, cyan, clipY1, clipY2); //Fill center complete rectangle
    } else {                      //If in unselected hollow state
        drawClippedRect(ui, x + c, y, w - 2*c, 2, cyan, clipY1, clipY2); //Top horizontal line
        drawClippedRect(ui, x + c, y + h - 2, w - 2*c, 2, cyan, clipY1, clipY2); //Bottom horizontal line
        drawClippedRect(ui, x, y + c, 2, h - 2*c, cyan, clipY1, clipY2); // Left vertical line
        drawClippedRect(ui, x + w - 2, y + c, 2, h - 2*c, cyan, clipY1, clipY2); // Right vertical line
        for(int i=0; i<c; ++i) {  //Draw pixel-level chamfered stair lines
            drawClippedRect(ui, x + c - i, y + i, 2, 2, cyan, clipY1, clipY2); 
            drawClippedRect(ui, x + w - c + i - 2, y + i, 2, 2, cyan, clipY1, clipY2); 
            drawClippedRect(ui, x + c - i, y + h - 1 - i, 2, 2, cyan, clipY1, clipY2); 
            drawClippedRect(ui, x + w - c + i - 2, y + h - 1 - i, 2, 2, cyan, clipY1, clipY2); 
        }
    }
}

// ============== MUSIC_LIST exclusive text drawing black tech with top/bottom occlusion ==============
/**
 * @brief Two-dimensional clipped dot-matrix text rendering exclusive to music list
 */
void UIRenderer::drawListText(FramebufferUI* ui, const std::string& text, int x, int y, Color color, int scale, int clipX1, int clipX2, int clipY1, int clipY2) {
    for (size_t i = 0; i < text.length(); ++i) {
        unsigned char c = text[i];
        if (c < 32 || c > 127) c = 63; 
        const unsigned char* glyph = font8x8[c - 32];
        for (int col = 0; col < 8; ++col) {
            int pixelX = x + (i * 8 + col) * scale;
            if (pixelX < clipX1 || pixelX + scale > clipX2) continue; 
            for (int row = 0; row < 8; ++row) {
                if (glyph[row] & (1 << (7 - col))) {
                    // Use low-level clipped drawing, neatly sliced at boundaries, consuming absolutely no redundant CPU
                    drawClippedRect(ui, pixelX, y + row * scale, scale, scale, color, clipY1, clipY2);
                }
            }
        }
    }
}

// =============== STANDBY interface exclusive render components ============
/**
 * @brief Render segmented gradient memory bar for standby interface
 */
//STANDBY exclusive: Gradient segmented memory bar
void UIRenderer::renderStandbyMemoryBar(FramebufferUI* ui, int x, int y, int w, int h, int percent) {
    int segments = 10;         //Divide into 10 data blocks
    int gap = 4;               //Physical gap between data blocks
    int segW = (w - (segments - 1) * gap) / segments;  //Rigorously calculate exact width of each color block

    int activeSegs = static_cast<int>(std::ceil((percent / 100.0f) * segments)); //Light up segments based on incoming usage rate

    for (int i = 0; i < segments; ++i) {     //Iterate segment slots
        Color col = {0, 80, 100};            //Default unactivated dark slot
        if (i < activeSegs) {                //State mapping engine
            if (i < 4) col = {0, 255, 255};           // Front 40%: Cyber cyan
            else if (i < 7) col = {255, 240, 150};    // 40%-70%: Warning beige
            else col = {255, 100, 100};               // 70% and above: Danger red
        }
        ui->drawRect(x + i * (segW + gap), y, segW, h, col); // Push data segment into physical VRAM area
    }
}

/**
 * @brief Drawing of pixel-style weather icons
 */
// STANDBY exclusive: drawing of pixel-style weather icons
void UIRenderer::renderWeatherIcon(FramebufferUI* ui, int x, int y, int weatherCode) {
    Color cyan = {0, 255, 255};         // Base cyan setting
    Color yellow = {255, 240, 150};     // Base yellow setting
    Color white = {255, 255, 255};      // Base white setting
    
    // 0: Sunny (SUNNY) - Don't draw clouds, only draw a big sun
    if (weatherCode == 0) {
        ui->drawRect(x + 25, y + 5, 20, 20, yellow); // Draw center core
        ui->drawRect(x + 33, y - 5, 4, 6, yellow);  // Top ray
        ui->drawRect(x + 33, y + 29, 4, 6, yellow); // Bottom ray
        ui->drawRect(x + 13, y + 13, 6, 4, yellow); // Left ray
        ui->drawRect(x + 51, y + 13, 6, 4, yellow); // Right ray
        return; // Return directly after drawing sunny
    }

    // Other weathers: all draw a basic cloud shape as a base
    ui->drawRect(x + 15, y, 30, 8, cyan);
    ui->drawRect(x + 5, y + 8, 50, 16, cyan);
    ui->drawRect(x, y + 24, 60, 8, cyan);

    // Add effects based on weather code
    if (weatherCode == 1) { 
        // 1: Cloudy (CLOUDY) - Draw a small sun partially occluded at top right of cloud
        ui->drawRect(x + 50, y - 5, 12, 12, yellow);
    } 
    else if (weatherCode == 2) { 
        // 2: Rainy (RAINY) - Slanted raindrops
        ui->drawRect(x + 10, y + 38, 4, 10, cyan);
        ui->drawRect(x + 30, y + 38, 4, 10, cyan);
        ui->drawRect(x + 50, y + 38, 4, 10, cyan);
    }
    else if (weatherCode == 3) { 
        // 3: Snowy (SNOWY) - Scattered white snowflake dots
        ui->drawRect(x + 15, y + 40, 6, 6, white);
        ui->drawRect(x + 38, y + 36, 6, 6, white);
        ui->drawRect(x + 25, y + 50, 6, 6, white);
        ui->drawRect(x + 48, y + 48, 6, 6, white);
    }
    else if (weatherCode == 4) { 
        // 4: Storm (STORM) - Cyber yellow lightning
        ui->drawRect(x + 28, y + 35, 8, 10, yellow);
        ui->drawRect(x + 22, y + 45, 10, 4, yellow);
        ui->drawRect(x + 24, y + 49, 6, 12, yellow);
    }
    else if (weatherCode == 5) { 
        // 5: Foggy (FOGGY) - Parallel mist lines below the cloud
        ui->drawRect(x + 5, y + 40, 50, 4, white);
        ui->drawRect(x + 15, y + 48, 40, 4, white);
        ui->drawRect(x + 10, y + 56, 30, 4, white);
    }
}

/**
 * @brief Render dynamic breathing lights for topology background nodes
 */
void UIRenderer::renderTopologyBreathingLights(FramebufferUI* ui, int cx, int cy, float phase) {
    // Calculate breathing intensity (0.0 - 1.0) using sin function
    float pulse = (std::sin(phase) + 1.0f) * 0.5f;
    int glow = static_cast<int>(pulse * 255.0f);   // Linear conversion to color gamut
    Color pulseCol = {0, (uint8_t)glow, 255};      // Breathing cyan

    // Define exact coordinate offsets [dx, dy] of 8 nodes relative to center point (cx, cy)
    // inner circle diagonal radius approx 42 (dx/dy approx 30), outer circle cross radius approx 78
    const int offsets[8][2] = {
        // 4 nodes of inner circle (Top-Left, Top-Right, Bottom-Left, Bottom-Right)
        {-30, -30}, { 30, -30},
        {-30,  30}, { 30,  30},

        // 4 nodes of outer circle (Up, Down, Left, Right)
        {  0, -78}, {  0,  78},
        {-78,   0}, { 78,   0}
    };

    // Iterate through these 8 fixed coordinate points, draw breathing lights
    for (int i = 0; i < 8; ++i) {
        int nx = cx + offsets[i][0];
        int ny = cy + offsets[i][1];
        // Draw a 6x6 light spot to perfectly cover the circle on the background
        ui->drawRect(nx - 3, ny - 3, 6, 6, pulseCol);
    }
}

} // namespace UI
