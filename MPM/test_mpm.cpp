#include <iostream>
#include <string>
#include <thread>
#include "../SystemInterfaces.hpp"
#include "include/WavDecoder.hpp"
#include "include/RingBuffer.hpp"
#include "include/MusicController.hpp"
#include "include/AudioEngine.hpp"

int main() {
    std::cout << "==========================================" << std::endl;
    std::cout << "🚀 桌面音乐空间站 - 交互式终端控制台 🚀" << std::endl;
    std::cout << "==========================================" << std::endl;

    // 1. 组装所有基础设施
    WavDecoder decoder;
    RingBuffer buffer(8192);
    MusicController controller(&decoder, &buffer);
    AudioEngine engine(&controller);

    // 2. 点火启动底层硬件引擎 (它会在后台默默待命)
    if (!engine.init("default", 44100)) {
        std::cerr << "❌ 硬件引擎初始化失败！" << std::endl;
        return -1;
    }
    engine.start();

    // 3. 设置初始歌单 (确保你的 build 目录里有这个文件)
    std::vector<std::string> playlist = {"test.wav"}; 
    controller.setPlaylist(playlist);

    std::cout << "✅ 底层引擎已就绪！\n" << std::endl;
    std::cout << "支持的指令: " << std::endl;
    std::cout << "  [p]  - 播放 / 暂停" << std::endl;
    std::cout << "  [+]  - 音量加" << std::endl;
    std::cout << "  [-]  - 音量减" << std::endl;
    std::cout << "  [n]  - 下一首" << std::endl;
    std::cout << "  [q]  - 退出程序" << std::endl;
    std::cout << "------------------------------------------" << std::endl;

    // 4. 前台交互循环：死盯着你的键盘输入
    std::string input;
    while (true) {
        std::cout << ">> ";
        std::cin >> input; // 程序会卡在这里等敲回车，但后台音乐不会停！

        System::ControlCommand cmd;
        
        if (input == "p") {
            cmd.type = System::CommandType::PLAY_PAUSE;
            controller.processCommand(cmd);
        } 
        else if (input == "+") {
            cmd.type = System::CommandType::VOLUME_UP;
            controller.processCommand(cmd);
        } 
        else if (input == "-") {
            cmd.type = System::CommandType::VOLUME_DOWN;
            controller.processCommand(cmd);
        } 
        else if (input == "n") {
            cmd.type = System::CommandType::NEXT_TRACK;
            controller.processCommand(cmd);
        } 
        else if (input == "q") {
            std::cout << "准备熄火，安全关闭引擎..." << std::endl;
            break; // 跳出死循环，准备结束程序
        } 
        else {
            std::cout << "⚠️ 未知指令！请使用 p, +, -, n, q" << std::endl;
        }
    }

    // 5. 安全退出
    engine.stop();
    std::cout << "👋 拜拜！已安全退出。" << std::endl;
    return 0;
}
