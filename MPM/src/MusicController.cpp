#include "../include/MusicController.hpp"
#include <iostream>
#include <algorithm>

// 🌟 构造函数：初始化 m_volume(1.0f) 代表默认 100% 音量
MusicController::MusicController(WavDecoder* decoder, RingBuffer* buffer)
    : m_decoder(decoder), m_ringBuffer(buffer), m_isPlaying(true),
      m_currentTrackIndex(0), m_volume(0.5f) {}

void MusicController::setPlaylist(const std::vector<std::string>& playlist) {
    m_playlist = playlist;
    if (!m_playlist.empty()) {
        m_currentTrackIndex = 0;
        m_decoder->open(m_playlist[m_currentTrackIndex]);
    }
}

void MusicController::processCommand(const System::ControlCommand& cmd) {
    switch (cmd.type) {
        case System::CommandType::PLAY_PAUSE:
            m_isPlaying = !m_isPlaying;
            std::cout << (m_isPlaying ? "▶️ 播放中" : "⏸️ 已暂停") << std::endl;
            break;

// 🌟 补回点播特定歌曲的逻辑！
        case System::CommandType::SELECT_SONG:
            // 确保传过来的歌曲索引 (cmd.intValue) 没有越界
            if (!m_playlist.empty() && cmd.intValue >= 0 && cmd.intValue < static_cast<int>(m_playlist.size())) {
                m_currentTrackIndex = cmd.intValue;
                m_ringBuffer->flush(); // 切歌前清空缓存，防止残音
                m_decoder->open(m_playlist[m_currentTrackIndex]); // 读取新歌
                m_isPlaying = true;    // 直接开播
                std::cout << "🔀 选歌播放: " << m_playlist[m_currentTrackIndex] << std::endl;
            }
            break;

        case System::CommandType::NEXT_TRACK:
            if (!m_playlist.empty()) {
                m_currentTrackIndex = (m_currentTrackIndex + 1) % m_playlist.size();
                m_ringBuffer->flush(); // 切歌防串音
                m_decoder->open(m_playlist[m_currentTrackIndex]);
                m_isPlaying = true;
                std::cout << "⏭️ 下一首: " << m_playlist[m_currentTrackIndex] << std::endl;
            }
            break;

        case System::CommandType::PREV_TRACK:
            if (!m_playlist.empty()) {
                m_currentTrackIndex = (m_currentTrackIndex == 0) ? 
                                      (m_playlist.size() - 1) : (m_currentTrackIndex - 1);
                m_ringBuffer->flush(); // 切歌防串音
                m_decoder->open(m_playlist[m_currentTrackIndex]);
                m_isPlaying = true;
                std::cout << "⏮️ 上一首: " << m_playlist[m_currentTrackIndex] << std::endl;
            }
            break;

        case System::CommandType::SEEK_FORWARD:
            if (m_decoder && !m_playlist.empty()) {
                m_ringBuffer->flush(); // 跳转防残音
                m_decoder->seekTo(cmd.floatValue);
            }
            break;

        // 🔊 真正生效的音量控制
        case System::CommandType::VOLUME_UP:
            m_volume = std::min(1.0f, m_volume + 0.05f);
            std::cout << "🔊 音量: " << static_cast<int>(m_volume * 100) << "%" << std::endl;
            break;

        case System::CommandType::VOLUME_DOWN:
            m_volume = std::max(0.0f, m_volume - 0.05f);
            std::cout << "🔉 音量: " << static_cast<int>(m_volume * 100) << "%" << std::endl;
            break;

        default:
            break;
    }
}

void MusicController::onProcessAudio(std::vector<float>& buffer, uint32_t numFrames) {
    if (m_isPlaying && m_decoder) {
        // 1. 获取原始音频数据
        m_decoder->onProcessAudio(buffer, numFrames);

        // 2. 🌟 物理外挂：给每一个声音样本乘上音量倍数
        for (size_t i = 0; i < buffer.size(); ++i) {
            buffer[i] *= m_volume;
        }
    } else {
        // 暂停时输出纯静音
        std::fill(buffer.begin(), buffer.end(), 0.0f);
    }
}
