#include <iostream>
#include <thread>
#include <chrono>
#include "../SystemInterfaces.hpp"
#include "include/WavDecoder.hpp"
#include "include/RingBuffer.hpp"
#include "include/MusicController.hpp"
#include "include/AudioEngine.hpp"

int main() {
    std::cout << "🚀 桌面音乐空间站 - 底层引擎全链路点火测试 🚀\n" << std::endl;

    // 1. 实例化你的基础设施
    WavDecoder decoder;
    RingBuffer buffer(8192); // 水池开大一点，8KB

    // 2. 实例化桥梁控制器，把你的设施交给他
    MusicController controller(&decoder, &buffer);

    // 3. 实例化队友的硬件引擎，把桥梁插进去
    AudioEngine engine(&controller);

    // 4. 启动 ALSA 硬件驱动 (此时扬声器被接管，准备随时发声)
    if (!engine.init("default", 44100)) { // 注意：test.wav 如果是 44100Hz 最好，其他采样率会被重采样
        std::cerr << "❌ 硬件引擎初始化失败！" << std::endl;
        return -1;
    }
    engine.start(); // 启动抽水机线程！

    // 5. 设置歌单并发送播放指令！
    std::vector<std::string> playlist = {"test.wav"}; // 确保 build 目录下有这个文件
    controller.setPlaylist(playlist);

    System::ControlCommand playCmd;
    playCmd.type = System::CommandType::PLAY_PAUSE;
    
    std::cout << "\n▶️ 发送播放指令..." << std::endl;
    controller.processCommand(playCmd); // 音乐应该在这一刻响起！

    // 6. 让主线程存活 10 秒钟，让你听听声！
    std::cout << "🎵 正在播放中，请戴上耳机/打开音箱听声音 (持续10秒)..." << std::endl;
    for(int i = 1; i <= 10; ++i) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
        std::cout << i << "s..." << std::flush << " ";
    }
    std::cout << "\n" << std::endl;

    // 7. 测试暂停功能
    std::cout << "⏸️ 发送暂停指令..." << std::endl;
    controller.processCommand(playCmd);
    std::this_thread::sleep_for(std::chrono::seconds(3)); // 享受 3 秒钟毫无爆音的完美宁静

    // 8. 安全关闭引擎
    engine.stop();
    std::cout << "✅ 测试圆满结束，引擎已安全关闭。" << std::endl;

    return 0;
}
