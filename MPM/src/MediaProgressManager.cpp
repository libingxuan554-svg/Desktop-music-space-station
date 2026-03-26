#include "../include/MediaProgressManager.hpp"
#include <iostream>

MediaProgressManager::MediaProgressManager(WavDecoder* dec, RingBuffer* buf) 
    : decoder(dec), ringBuffer(buf) {
    std::cout << "[ProgressManager] Initialized and mapped to System Interfaces." << std::endl;
}

// ---------------------------------------------------------
// 处理 UI 传来的标准化指令
// ---------------------------------------------------------
bool MediaProgressManager::processCommand(const System::ControlCommand& cmd) {
    if (!decoder || !ringBuffer) return false;

    // 我们只拦截跟“进度”相关的指令，其他的放行让 AudioEngine 处理
    switch (cmd.type) {
        case System::CommandType::SEEK_FORWARD:
        case System::CommandType::SEEK_BACKWARD: {
            // 队友定义了用 intValue 或者 floatValue 传参数。
            // 这里假设 UI 会把目标秒数塞进 intValue 里传过来
            double targetTimeSeconds = static_cast<double>(cmd.intValue);
            
            std::cout << "[ProgressManager] Received SEEK Command: Jump to " << targetTimeSeconds << "s" << std::endl;
            
            if (decoder->seekToTime(targetTimeSeconds)) {
                ringBuffer->flush(); // 清空旧水池！
                return true;
            }
            return false;
        }
        
        default:
            // 像 PLAY_PAUSE, VOLUME_UP 等指令是音频引擎控制的，我们不管
            return false; 
    }
}

// ---------------------------------------------------------
// 组装数据给 UI：获取当前播放状态
// ---------------------------------------------------------
void MediaProgressManager::injectTimeData(System::PlaybackStatus& status) {
    if (!decoder) return;

    // 直接调用底层引擎，获取精准秒数，塞进队友要求的变量名里！
    status.currentPosition = decoder->getCurrentPosition();
    status.totalDuration   = decoder->getTotalDuration();
}
