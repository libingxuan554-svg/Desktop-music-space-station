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
	// (New: Apply scrollOffset to Y-coordinates for song list rendering)
    int currentOffset = InteractionManager::scrollOffset.load();
    /*renderMirrorEqualizer(ui, audioIntensity, ui->getWidth() / 2, 200, 100);*/
	
	for (const auto& btn : buttons) {
        // (Optimization: Scissor test - do not render buttons that are off-screen)
        if (btn.y + btn.h < 0 || btn.y > ui->getHeight()) continue;

        // (Correction: Draw button body with its defined color)
        ui->drawRect(btn.x, btn.y, btn.w, btn.h, btn.color);
    
		// (New: Visual feedback - Draw a highlight border if this is the currently playing song)
        if (InteractionManager::currentPage == UIPage::MUSIC_LIST && btn.id == "SELECTED_SONG") {
            ui->drawRect(btn.x, btn.y, btn.w, 5, {255, 255, 255}); 
        }
    }
	
	// Render visualizer only in PLAYER page
    if (InteractionManager::currentPage == UIPage::PLAYER) {
        renderMirrorEqualizer(ui, audioIntensity, ui->getWidth() / 2, 200, 100);
        
        // (New: Render Progress Bar based on PlaybackStatus)
        float progress = 0.0f;
        if (InteractionManager::currentStatus.totalDuration > 0) {
            progress = (float)InteractionManager::currentStatus.currentPosition / InteractionManager::currentStatus.totalDuration;
        }
        int progressW = static_cast<int>(progress * (ui->getWidth() - 200));
        ui->drawRect(100, 350, progressW, 10, {0, 150, 255}); 
    }
}
}