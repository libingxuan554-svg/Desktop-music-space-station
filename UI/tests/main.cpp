#include "FramebufferUI.hpp"
#include "TouchHandler.hpp"
#include "InteractionManager.hpp"
#include "SystemInterfaces.hpp"
#include <iostream>
#include <chrono>
#include <thread>

using namespace UI;

int main() {
    try {
        // 1. 初始化底层驱动 (默认使用 /dev/fb0)
        FramebufferUI ui("/dev/fb0");
        std::cout << "Framebuffer initialized: " << ui.getWidth() << "x" << ui.getHeight() << std::endl;

        // 2. 初始化触摸处理器 (确保路径与树莓派识别的节点一致)
        TouchHandler touch("/dev/input/event0"); 

        // 3. 设置命令发射器 (模拟 FastDDS 发送指令)
        InteractionManager::setCommandEmitter([](const System::ControlCommand& cmd) {
            std::cout << "[Command Sent] Type: " << static_cast<int>(cmd.type) 
                      << " Value: " << cmd.value << std::endl;
        });

        // 4. 启动触摸监听，并将点击事件传递给 InteractionManager
        touch.startListening([](int x, int y) {
            InteractionManager::handleTouch(x, y);
        });

        // 5. 模拟主循环：模拟数据输入并刷新 UI
        bool running = true;
        float fakeIntensity = 0.0f;
        float step = 0.05f;

        std::cout << "Starting main loop. Press Ctrl+C to stop." << std::endl;

        while (running) {
            // 模拟音频跳动数据
            fakeIntensity += step;
            if (fakeIntensity > 1.0f || fakeIntensity < 0.0f) step = -step;

            // 模拟系统状态
            System::AudioVisualData avData = { std::abs(fakeIntensity) };
            System::PlaybackStatus status = { true, 120, 45, "Testing Track" };
            System::EnvironmentStatus env = { 24.5f, "Sunny", "2026-03-20" };

            // 根据当前页面调用对应的刷新函数
            if (InteractionManager::currentPage == UIPage::STANDBY) {
                ui.refreshStandbyMode(env);
            } else {
                ui.refreshMusicAnimation(avData, status);
            }

            // 控制刷新率约为 30 FPS
            std::this_thread::sleep_for(std::chrono::milliseconds(33));
        }

    } catch (const std::exception& e) {
        std::cerr << "Runtime Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}