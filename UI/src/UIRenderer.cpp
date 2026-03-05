#include "UIRenderer.hpp"
#include "FramebufferUI.hpp"
#include <algorithm>

namespace UI {

void UIRenderer::renderTechBar(FramebufferUI* ui, float value, int x, int y, int maxW) {
    ui->clear({10, 10, 30}); // 底层驱动清除屏幕
    int barLen = std::min(static_cast<int>(value * (maxW / 50.0f)), maxW);
    ui->drawRect(x, y, barLen, 40, {255, 100, 0}); // 底层驱动绘制矩形
}

void UIRenderer::renderMirrorEqualizer(FramebufferUI* ui, float intensity, int cx, int cy, int maxH) {
    int h = static_cast<int>(intensity * maxH);
    ui->drawRect(cx - 60, cy - h, 40, h * 2, {0, 255, 150}); // 底层驱动绘制音柱
}

}