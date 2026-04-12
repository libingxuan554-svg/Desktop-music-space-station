#include "../include/MediaProgressManager.hpp"
#include <iostream>

// 构造函数：关联解码器
MediaProgressManager::MediaProgressManager(WavDecoder* dec) : decoder(dec) {}

// 处理来自 UI 的进度控制指令
bool MediaProgressManager::processCommand(const System::ControlCommand& cmd) {
    if (!decoder) return false;

    // 1. 如果指令是百分比跳转 (0.0 ~ 1.0)
    if (cmd.type == System::CommandType::SEEK_FORWARD) {
        decoder->seekTo(cmd.floatValue);
        return true;
    }

    // 2. 如果指令是具体秒数跳转
    if (cmd.type == System::CommandType::SEEK_BACKWARD) {
        return decoder->seekToTime(static_cast<double>(cmd.floatValue));
    }

    return false;
}

// 🌟 核心功能：注入时间数据，供 UI 显示进度条
void MediaProgressManager::injectTimeData(System::PlaybackStatus& status) {
    if (!decoder) return;

    // 1. 获取当前秒数
    status.currentPosition = static_cast<float>(decoder->getCurrentPosition());

    // 2. 获取总时长秒数
    status.totalDuration = static_cast<float>(decoder->getTotalDuration());

    // 3. 注意：因为 System::PlaybackStatus 结构体里没有 'progress' 成员，
    // 我们不需要在这里计算百分比，UI 会根据 currentPosition / totalDuration 自己算。
}
