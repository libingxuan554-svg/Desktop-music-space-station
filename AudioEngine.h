#pragma once
#include <string>
#include <vector>
#include <atomic>
#include <mutex>

#include "RingBuffer.h"
#include "miniaudio.h"

class AudioEngine {
public:
    enum class PlaybackState {
        Stopped,
        Playing,
        Paused
    };

    AudioEngine();
    ~AudioEngine();

    bool init(ma_uint32 sampleRate = 48000,
              ma_uint32 channels = 2,
              ma_uint32 analysisWindowFrames = 2048,
              size_t ringBufferSeconds = 2);

    bool load(const std::string& file);
    void play();
    void pause();
    void stop();
    void setVolume(float v);

    float getVolume() const;
    PlaybackState getState() const;
    bool isLoaded() const;

    // 返回最近一段 PCM 数据，长度 = analysisWindowFrames * channels
    bool getPcmChunk(std::vector<float>& outChunk);

    ma_uint32 getSampleRate() const;
    ma_uint32 getChannels() const;

private:
    static void dataCallback(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount);
    void onAudioCallback(float* output, ma_uint32 frameCount);
    void fillSilence(float* output, ma_uint32 frameCount);

private:
    ma_device m_device{};
    ma_decoder m_decoder{};

    bool m_initialized = false;
    bool m_decoderInitialized = false;

    std::atomic<bool> m_deviceStarted{false};
    std::atomic<bool> m_fileLoaded{false};
    std::atomic<float> m_volume{1.0f};
    std::atomic<PlaybackState> m_state{PlaybackState::Stopped};

    std::mutex m_decoderMutex;

    RingBuffer m_ringBuffer;

    ma_uint32 m_sampleRate = 48000;
    ma_uint32 m_channels = 2;
    ma_uint32 m_analysisWindowFrames = 2048;

    std::string m_currentFile;
};
