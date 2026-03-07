#include "UIRenderer.hpp"
#include "FramebufferUI.hpp"
#include "InteractionManager.hpp" //包含以访问 UIButton 成员
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


void UIRenderer::renderButtons(FramebufferUI* ui, const std::vector<UIButton>& buttons, float audioIntensity) {
    // 1. 使用传入的参数渲染音频条
    renderMirrorEqualizer(ui, audioIntensity, ui->getWidth() / 2, 200, 100);
    
    // 2. 渲染交互按钮
    for (const auto& btn : buttons) {
        ui->drawRect(btn.x, btn.y, btn.w, btn.h, btn.color);
        // 此处可扩展绘制文字
    }
}
}

}