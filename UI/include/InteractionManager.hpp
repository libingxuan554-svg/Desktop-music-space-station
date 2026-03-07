#ifndef INTERACTION_MANAGER_HPP
#define INTERACTION_MANAGER_HPP

#include <vector>
#include <string>
#include <functional>
#include <atomic> 
#include "FramebufferUI.hpp"

namespace UI {
    // 定义当前所处的界面状态
    enum class UIPage { STANDBY, MUSIC_LIST, PLAYER };

    struct UIButton {
        int x, y, w, h;
        std::string id;
        Color color;
        std::function<void()> action;
    };

    class InteractionManager {
    public:
        static std::atomic<UIPage> currentPage;
        static std::atomic<int> scrollOffset; // 用于歌单上下滑动的偏移量

        // 获取当前界面应显示的按钮布局
        static std::vector<UIButton> getActiveLayout();
        
        // 处理点击判定
        static void handleTouch(int x, int y);
        
        // 处理滑动逻辑（用于歌单）
        static void handleScroll(int deltaY);
    };
}
#endif