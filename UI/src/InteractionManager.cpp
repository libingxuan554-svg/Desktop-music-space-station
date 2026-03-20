#include "InteractionManager.hpp"
#include <iostream>

namespace UI {
    std::atomic<UIPage> InteractionManager::currentPage{UIPage::STANDBY};
    std::atomic<int> InteractionManager::scrollOffset{0};

    std::vector<UIButton> InteractionManager::getActiveLayout() {
        std::vector<UIButton> layout;
        
        if (currentPage == UIPage::MUSIC_LIST) {
            // 界面1：歌单目录（支持点击切歌并进入播放器）
            for(int i=0; i<5; ++i) {
                layout.push_back({50, 60 * i + scrollOffset, 600, 50, "SONG_" + std::to_string(i), {40, 40, 60}, [](){
                    currentPage = UIPage::PLAYER; 
                }});
            }
        } else if (currentPage == UIPage::PLAYER) {
            // 界面2：播放界面（上下曲、播放暂停、快进快退）
            layout.push_back({100, 400, 80, 60, "PREV", {100, 100, 100}, [](){ /* 上一曲 */ }});
            layout.push_back({250, 400, 100, 60, "PLAY", {0, 200, 0}, [](){ /* 暂停/播放 */ }});
            layout.push_back({450, 400, 80, 60, "NEXT", {100, 100, 100}, [](){ /* 下一曲 */ }});
        }
        return layout;
    }

    void InteractionManager::handleTouch(int x, int y) {
        auto buttons = getActiveLayout();
        for (const auto& btn : buttons) {
            if (x >= btn.x && x <= btn.x + btn.w && y >= btn.y && y <= btn.y + btn.h) {
                btn.action();
                break;
            }
        }
    }

    void InteractionManager::handleScroll(int deltaY) {
        if (currentPage == UIPage::MUSIC_LIST) {
            scrollOffset += deltaY; // 更新滑动偏移量
        }
    }
}