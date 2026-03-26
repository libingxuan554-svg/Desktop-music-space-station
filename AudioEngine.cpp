#define MINIAUDIO_IMPLEMENTATION
#include "miniaudio.h"

#include "AudioEngine.h"
#include <algorithm>
#include <cstring>
#include <iostream>

AudioEngine::AudioEngine() {}

AudioEngine::~AudioEngine() {
    // 先关闭设备，确保回调线程结束
    if (m_initialized) {
        ma_device_uninit(&m_device);
        m_initialized = false;
    }

    // 再释放解码器
    std::lock_guard<std::mutex> lock(m_decoderMutex);
    if (m_decoderInitialized) {
        ma_decoder_uninit(&m_decoder);
        m_decoderInitialized = false;
    }
}

bool AudioEngine::init(ma_uint32 sampleRate,
                       ma_uint32 channels,
                       ma_uint32 analysisWindowFrames,
                       size_t ringBufferSeconds) {
    if (m_initialized) {
        return true;
    }

    m_sampleRate = sampleRate;
    m_channels = channels;
    m_analysisWindowFrames = analysisWindowFrames;

    ma_device_config deviceConfig = ma_device_config_init(ma_device_type_playback);
    deviceConfig.playback.format = ma_format_f32;          // 输出 float PCM
    deviceConfig.playback.channels = channels;             // 立体声
    deviceConfig.sampleRate = sampleRate;                  // 48000 Hz
    deviceConfig.dataCallback = AudioEngine::dataCallback;
    deviceConfig.pUserData = this;

    ma_result result = ma_device_init(nullptr, &deviceConfig, &m_device);
    if (result != MA_SUCCESS) {
        std::cerr << "[AudioEngine] Failed to initialize playback device." << std::endl;
        return false;
    }

    // 设备实际参数
    m_sampleRate = m_device.sampleRate;
    m_channels = m_device.playback.channels;

    // 环形缓冲区容量 = sampleRate * channels * seconds
    size_t ringCapacity = static_cast<size_t>(m_sampleRate) * m_channels * ringBufferSeconds;
    m_ringBuffer.resize(ringCapacity);

    m_initialized = true;
    return true;
}

bool AudioEngine::load(const std::string& file) {
    if (!m_initialized) {
        std::cerr << "[AudioEngine] Device is not initialized." << std::endl;
        return false;
    }

    std::lock_guard<std::mutex> lock(m_decoderMutex);

    if (m_decoderInitialized) {
        ma_decoder_uninit(&m_decoder);
        m_decoderInitialized = false;
    }

    ma_decoder_config decoderConfig = ma_decoder_config_init(
        ma_format_f32,
        m_channels,
        m_sampleRate
    );

    ma_result result = ma_decoder_init_file(file.c_str(), &decoderConfig, &m_decoder);
    if (result != MA_SUCCESS) {
        std::cerr << "[AudioEngine] Failed to load file: " << file << std::endl;
        m_fileLoaded.store(false);
        return false;
    }

    m_decoderInitialized = true;
    m_fileLoaded.store(true);
    m_state.store(PlaybackState::Stopped);
    m_currentFile = file;
    m_ringBuffer.clear();

    std::cout << "[AudioEngine] Loaded file: " << file << std::endl;
    return true;
}

void AudioEngine::play() {
    if (!m_initialized || !m_fileLoaded.load()) {
        return;
    }

    if (!m_deviceStarted.load()) {
        ma_result result = ma_device_start(&m_device);
        if (result != MA_SUCCESS) {
            std::cerr << "[AudioEngine] Failed to start playback device." << std::endl;
            return;
        }
        m_deviceStarted.store(true);
    }

    m_state.store(PlaybackState::Playing);
}

void AudioEngine::pause() {
    if (!m_initialized) {
        return;
    }

    if (m_state.load() == PlaybackState::Playing) {
        m_state.store(PlaybackState::Paused);
    }
}

void AudioEngine::stop() {
    if (!m_initialized) {
        return;
    }

    m_state.store(PlaybackState::Stopped);
    m_ringBuffer.clear();

    std::lock_guard<std::mutex> lock(m_decoderMutex);
    if (m_decoderInitialized) {
        ma_decoder_seek_to_pcm_frame(&m_decoder, 0);
    }
}

void AudioEngine::setVolume(float v) {
    v = std::clamp(v, 0.0f, 1.0f);
    m_volume.store(v);
}

float AudioEngine::getVolume() const {
    return m_volume.load();
}

AudioEngine::PlaybackState AudioEngine::getState() const {
    return m_state.load();
}

bool AudioEngine::isLoaded() const {
    return m_fileLoaded.load();
}

ma_uint32 AudioEngine::getSampleRate() const {
    return m_sampleRate;
}

ma_uint32 AudioEngine::getChannels() const {
    return m_channels;
}

bool AudioEngine::getPcmChunk(std::vector<float>& outChunk) {
    size_t sampleCount = static_cast<size_t>(m_analysisWindowFrames) * m_channels;
    return m_ringBuffer.readLatest(outChunk, sampleCount);
}

void AudioEngine::dataCallback(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount) {
    (void)pInput;

    AudioEngine* engine = reinterpret_cast<AudioEngine*>(pDevice->pUserData);
    if (engine == nullptr) {
        return;
    }

    float* output = reinterpret_cast<float*>(pOutput);
    engine->onAudioCallback(output, frameCount);
}

void AudioEngine::fillSilence(float* output, ma_uint32 frameCount) {
    std::memset(output, 0, sizeof(float) * frameCount * m_channels);
}

void AudioEngine::onAudioCallback(float* output, ma_uint32 frameCount) {
    fillSilence(output, frameCount);

    if (m_state.load() != PlaybackState::Playing || !m_fileLoaded.load()) {
        return;
    }

    ma_uint64 framesRead = 0;

    {
        std::lock_guard<std::mutex> lock(m_decoderMutex);

        if (!m_decoderInitialized) {
            return;
        }

        ma_result result = ma_decoder_read_pcm_frames(&m_decoder, output, frameCount, &framesRead);

        // 解码失败且完全没读到数据，直接静音返回
        if (result != MA_SUCCESS && framesRead == 0) {
            return;
        }

        // 如果读到的帧不够，说明歌曲到末尾了
        if (framesRead < frameCount) {
            std::memset(
                output + framesRead * m_channels,
                0,
                sizeof(float) * (frameCount - framesRead) * m_channels
            );

            // 到结尾后停下，并把读指针复位到开头
            m_state.store(PlaybackState::Stopped);
            ma_decoder_seek_to_pcm_frame(&m_decoder, 0);
        }
    }

    // 音量缩放
    float volume = m_volume.load();
    size_t validSamples = static_cast<size_t>(framesRead) * m_channels;

    for (size_t i = 0; i < validSamples; ++i) {
        output[i] *= volume;
    }

    // 把“正在播放”的 PCM 数据写入环形缓冲区
    if (validSamples > 0) {
        m_ringBuffer.push(output, validSamples);
    }
}
