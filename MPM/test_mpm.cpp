#include <iostream>
#include "include/WavDecoder.hpp"
#include "include/RingBuffer.hpp"
#include "include/MediaProgressManager.hpp"
#include "include/SystemInterfaces.hpp"

int main() {
    std::cout << "========== MPM 模块独立测试 ==========" << std::endl;

    // 1. 搭建你的核心组件
    RingBuffer buffer(4096);
    WavDecoder decoder;
    MediaProgressManager manager(&decoder, &buffer);

    // 2. 模拟打开一首歌曲
    std::cout << "\n[测试 1] 尝试打开 test.wav..." << std::endl;
    // 注意：你运行测试的目录下必须真的有一个叫 test.wav 的文件！
    if (!decoder.openFile("test.wav")) {
        std::cerr << "❌ 打开失败！请确保当前目录下有一个 test.wav 文件用来测试。" << std::endl;
        return -1;
    }

    // 3. 模拟队友 (UI) 想要获取初始播放进度
    std::cout << "\n[测试 2] 获取初始播放状态..." << std::endl;
    System::PlaybackStatus status;
    manager.injectTimeData(status);
    std::cout << "-> 歌曲总时长: " << status.totalDuration << " 秒" << std::endl;
    std::cout << "-> 当前进度: " << status.currentPosition << " 秒" << std::endl;

    // 4. 模拟队友 (UI) 拖动进度条，要求快进到第 60 秒
    std::cout << "\n[测试 3] 模拟 UI 发送快进指令 (跳转到 60 秒)..." << std::endl;
    System::ControlCommand cmd;
    cmd.type = System::CommandType::SEEK_FORWARD;
    cmd.intValue = 60; // 目标 60 秒

    if (manager.processCommand(cmd)) {
        std::cout << "✅ 指令执行成功！水池已清空，解码器已跳转！" << std::endl;
    } else {
        std::cerr << "❌ 指令执行失败！" << std::endl;
    }

    // 5. 再次获取进度，验证是否真的跳到了 60 秒！
    std::cout << "\n[测试 4] 验证跳转后的状态..." << std::endl;
    manager.injectTimeData(status);
    std::cout << "-> 新的当前进度: " << status.currentPosition << " 秒" << std::endl;

    std::cout << "======================================" << std::endl;
    return 0;
}
