#ifndef MUSIC_CONTROLLER_HPP
#define MUSIC_CONTROLLER_HPP

#include <vector>
#include <string>
#include <atomic>
#include "../../SystemInterfaces.hpp"
#include "WavDecoder.hpp"
#include "RingBuffer.hpp"
#include "AudioSource.hpp"

class MusicController : public AudioSource {
public:
    MusicController(WavDecoder* decoder, RingBuffer* buffer);

    void setPlaylist(const std::vector<std::string>& playlist);
    void processCommand(const System::ControlCommand& cmd); // 确保返回 void
    void onProcessAudio(std::vector<float>& buffer, uint32_t numFrames) override;
//  新增：向外暴露当前的播放状态
    bool isPlaying() const { return m_isPlaying; }
//  新增：获取当前播放曲目的索引
    int getCurrentTrackIndex() const {
        return m_currentTrackIndex;
    }
//  新增：获取当前音量值 (0.0 - 1.0)
    float getVolume() const {
        return m_volume;
    }

private:
    WavDecoder* m_decoder;            // 对应报错的缺失字段
    RingBuffer* m_ringBuffer;         // 对应报错的缺失字段
    std::vector<std::string> m_playlist; // 对应报错的缺失字段
    std::atomic<bool> m_isPlaying;    // 对应报错的缺失字段
    int m_currentTrackIndex;          // 对应报错的缺失字段
    float m_volume;
};

#endif
