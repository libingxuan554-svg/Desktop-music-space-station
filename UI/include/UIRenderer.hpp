#ifndef UI_RENDERER_HPP
#define UI_RENDERER_HPP

#include <vector>
#include <string>
#include <functional>
#include "FramebufferUI.hpp"

namespace UI {

struct UIButton {
    int x, y, w, h;
    std::string id;
    Color color;
    std::function<void()> action;
};

class UIRenderer {
public:
    static void renderTechBar(FramebufferUI* ui, float value, int x, int y, int maxW);
    static void renderMirrorEqualizer(FramebufferUI* ui, float intensity, int cx, int cy, int maxH);
    static void renderMultiBarEQ(FramebufferUI* ui, float intensity, int cx, int cy, int maxH);

    // ============ PLAYER 专属组件 (冻结绝对安全) ============
    static void renderButton(FramebufferUI* ui, const UIButton& btn, bool isHighlighted = false);
    static void drawText(FramebufferUI* ui, const std::string& text, int x, int y, Color color, int scale = 2, int clipX1 = 0, int clipX2 = 1024);
    static void renderProgressBar(FramebufferUI* ui, float progress, int x, int y, int w, int h);

    // ============ MUSIC_LIST 专属组件 (冻结绝对安全) ============
    static void renderListButton(FramebufferUI* ui, int x, int y, int w, int h, bool isSelected, int clipY1, int clipY2);
    static void drawListText(FramebufferUI* ui, const std::string& text, int x, int y, Color color, int scale, int clipX1, int clipX2, int clipY1, int clipY2);
    
    // ============ STANDBY 专属组件 (新增，绝对隔离) ============
    static void renderStandbyMemoryBar(FramebufferUI* ui, int x, int y, int w, int h, int percent);
    static void renderWeatherIcon(FramebufferUI* ui, int x, int y, int weatherCode);
    static void renderTopologyBreathingLights(FramebufferUI* ui, int cx, int cy, float phase);

};

}
#endif
