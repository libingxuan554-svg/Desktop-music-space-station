#include "../include/MusicController.hpp"
#include <iostream>
#include <algorithm> // 必须引入这个来使用 std::fill, std::min, std::max

MusicController::MusicController(WavDecoder* dec, RingBuffer* buf) 
    : decoder(dec), ringBuffer(buf) {}

void MusicController::setPlaylist(const std::vector<std::string>& list) {
    playlist = list;
    currentTrackIndex = 0;
    if (!playlist.empty()) {
        loadCurrentTrack();
    }
}

void MusicController::loadCurrentTrack() {
    if (playlist.empty()) return;
    
    isPlaying = false; // 加载时先暂停，防止爆音
    if (decoder->openFile(playlist[currentTrackIndex])) {
        ringBuffer->flush(); // 清空上一首歌的残余水流
        std::cout << "🎵 成功加载歌曲: " << playlist[currentTrackIndex] << std::endl;
        isPlaying = true;    // 加载成功，自动开始播放
    } else {
        std::cerr << "❌ 无法打开文件: " << playlist[currentTrackIndex] << std::endl;
    }
}

// ==========================================
// 指令神经中枢：处理切歌、音量、播放暂停
// ==========================================
bool MusicController::processCommand(const System::ControlCommand& cmd) {
    switch (cmd.type) {
        case System::CommandType::PLAY_PAUSE:
            isPlaying = !isPlaying;
            std::cout << (isPlaying ? "▶️ 播放" : "⏸️ 暂停") << std::endl;
            return true;

        case System::CommandType::VOLUME_UP:
            volume = std::min(1.0f, volume.load() + 0.1f);
            std::cout << "🔊 音量增加: " << volume * 100 << "%" << std::endl;
            return true;

        case System::CommandType::VOLUME_DOWN:
            volume = std::max(0.0f, volume.load() - 0.1f);
            std::cout << "🔉 音量减少: " << volume * 100 << "%" << std::endl;
            return true;

        case System::CommandType::NEXT_TRACK:
            if (!playlist.empty()) {
                currentTrackIndex = (currentTrackIndex + 1) % playlist.size();
                loadCurrentTrack();
            }
            return true;

        case System::CommandType::PREV_TRACK:
            if (!playlist.empty()) {
                // 防止出现负数索引
                currentTrackIndex = (currentTrackIndex - 1 + playlist.size()) % playlist.size();
                loadCurrentTrack();
            }
            return true;

        default:
            return false;
    }
}

// ==========================================
// 核心供弹机：对接队友的 AudioEngine
// ==========================================
void MusicController::onProcessAudio(std::vector<float>& buffer, uint32_t numFrames) {
    // 1. 如果是暂停状态，我们给引擎喂 0.0f (绝对静音)，防止 ALSA 崩溃
    if (!isPlaying) {
        std::fill(buffer.begin(), buffer.end(), 0.0f);
        return;
    }

    // 2. 计算需要从你的水池抽多少字节
    // numFrames 是左右耳的对数。2 个声道，每个样本是 16 位的 int (占用 2 字节)
    size_t bytesNeeded = numFrames * 2 * sizeof(int16_t);
    std::vector<uint8_t> rawBytes(bytesNeeded, 0);

    // 3. 从你的 RingBuffer 抽水
    size_t bytesRead = ringBuffer->read(rawBytes.data(), bytesNeeded);
    size_t samplesRead = bytesRead / sizeof(int16_t);

    // 4. 硬核数据转换：16位整型 -> 32位浮点 -> 乘以音量！
    int16_t* pcm16 = reinterpret_cast<int16_t*>(rawBytes.data());

    for (size_t i = 0; i < buffer.size(); ++i) {
        if (i < samplesRead) {
            // 将 -32768~32767 的整数，压缩到 -1.0~1.0 的浮点数，并乘以音量
            buffer[i] = (pcm16[i] / 32768.0f) * volume;
        } else {
            // 如果水池没水了（解码跟不上），填充 0 防止爆音
            buffer[i] = 0.0f; 
        }
    }
}
