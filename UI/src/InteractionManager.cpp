#include "InteractionManager.hpp"
#include <iostream>

namespace UI {
	//静态成员初始化
    std::atomic<UIPage> InteractionManager::currentPage{UIPage::STANDBY};
    std::atomic<int> InteractionManager::scrollOffset{0};
	InteractionManager::CommandEmitter InteractionManager::commandEmitter = nullptr;
    System::PlaybackStatus InteractionManager::currentStatus;
    System::EnvironmentStatus InteractionManager::currentEnv;

    void InteractionManager::setCommandEmitter(CommandEmitter emitter) {
        commandEmitter = emitter;
    }

    std::vector<UIButton> InteractionManager::getActiveLayout() {
        std::vector<UIButton> layout;
        
        if (currentPage == UIPage::MUSIC_LIST) {
			// (New: Get current scrolling position)
            int offset = scrollOffset.load();
			
            // 界面1：歌单目录（支持点击切歌并进入播放器）
            for(int i=0; i<10; ++i) { // Assume 10 songs
                int btnY = 60 * i + offset + 50; 
                layout.push_back({50, btnY, 700, 50, "SONG_" + std::to_string(i), {40, 40, 60}, [i, this](){
                    // (Action: Send command to play specific song index)
                    if(commandEmitter) commandEmitter({System::CommandType::SELECT_SONG, i, 0.0f});
                    currentPage = UIPage::PLAYER; 
                }});
            }
        }
        else if (currentPage == UIPage::PLAYER) {
            // 界面2：播放界面：标准控制逻辑映射到协议指令
            layout.push_back({100, 400, 80, 60, "PREV", {100, 100, 100}, [](){/* 上一曲 */  
				if(commandEmitter) commandEmitter({System::CommandType::PREV_TRACK, 0, 0.0f});
			}});
            layout.push_back({250, 400, 100, 60, "PLAY", {0, 200, 0}, [](){ /* 暂停/播放 */ 
				if(commandEmitter) commandEmitter({System::CommandType::PLAY_PAUSE, 0, 0.0f});
			}});
            layout.push_back({450, 400, 80, 60, "NEXT", {100, 100, 100}, [](){ /* 下一曲 */ 
				if(commandEmitter) commandEmitter({System::CommandType::NEXT_TRACK, 0, 0.0f});
			}});
			// (New: Return button to go back to list)
            layout.push_back({10, 10, 80, 40, "BACK", {80, 20, 20}, [](){
                currentPage = UIPage::MUSIC_LIST;
            }});
        }
        return layout;
    }

    void InteractionManager::handleTouch(int x, int y) {
        auto layout = getActiveLayout();
        for (const auto& btn : layout) {
            if (x >= btn.x && x <= btn.x + btn.w && y >= btn.y && y <= btn.y + btn.h) {
                if (btn.action)btn.action();
                break;
            }
        }
    }

    void InteractionManager::handleScroll(int deltaY) {
        // (Optimization: Cumulative scrolling with boundary limits)
        int newOffset = scrollOffset.load() + deltaY;
        
        // (New: Prevent scrolling too far up or down)
        if (newOffset > 0) newOffset = 0; 
        if (newOffset < -500) newOffset = -500; // Limit based on list height
        
        scrollOffset.store(newOffset);
    }
}