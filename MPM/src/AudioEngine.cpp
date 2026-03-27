#include "../include/AudioEngine.hpp"
#include "../include/HardwareController.hpp"
#include <alsa/asoundlib.h>
#include <iostream>
#include <cmath>
#include <algorithm>
#include <vector>

// ==========================================
// 1. 核心算法：Goertzel 频谱提取 (提取特定频率能量)
// ==========================================
float goertzelMagnitude(int numSamples, int targetFreq, int sampleRate, const float* data) {
    float k = 0.5f + ((float)numSamples * targetFreq) / sampleRate;
    float omega = (2.0f * M_PI * k) / numSamples;
    float sine = std::sin(omega);
    float cosine = std::cos(omega);
    float coeff = 2.0f * cosine;
    float q0 = 0, q1 = 0, q2 = 0;

    for (int i = 0; i < numSamples; ++i) {
        q0 = coeff * q1 - q2 + data[i];
        q2 = q1;
        q1 = q0;
    }
    return std::sqrt(q1 * q1 + q2 * q2 - q1 * q2 * coeff);
}

// ==========================================
// 2. 引擎构造与生命周期管理
// ==========================================
AudioEngine::AudioEngine(AudioSource* src, HardwareController* hw) 
    : source(src), hwController(hw), pcmHandle(nullptr), running(false) {}

AudioEngine::~AudioEngine() {
    stop();
}

bool AudioEngine::init(const char* device, unsigned int sampleRate) {
    int err;
    // 打开 ALSA PCM 播放设备
    if ((err = snd_pcm_open(&pcmHandle, device, SND_PCM_STREAM_PLAYBACK, 0)) < 0) {
        std::cerr << "ALSA 音频设备打开失败: " << snd_strerror(err) << std::endl;
        return false;
    }

    // 设置硬件参数 (16位小端, 交错模式, 立体声, 采样率)
    if ((err = snd_pcm_set_params(pcmHandle, SND_PCM_FORMAT_S16_LE, SND_PCM_ACCESS_RW_INTERLEAVED, 2, sampleRate, 1, 500000)) < 0) {
        std::cerr << "ALSA 参数设置失败: " << snd_strerror(err) << std::endl;
        return false;
    }
    return true;
}

void AudioEngine::start() {
    if (running) return;
    running = true;
    playbackThread = std::thread(&AudioEngine::playbackWorker, this);
}

void AudioEngine::stop() {
    running = false;
    if (playbackThread.joinable()) {
        playbackThread.join();
    }
    if (pcmHandle) {
        snd_pcm_close(pcmHandle);
        pcmHandle = nullptr;
    }
}

void AudioEngine::recoverFromError(int err) {
    if (err == -EPIPE) {
        snd_pcm_prepare(pcmHandle); // 恢复音频欠载 (XRUN)
    }
}

// ==========================================
// 3. 核心音频工厂 (截取数据 -> 算频谱 -> 发信箱 -> 播声音)
// ==========================================
void AudioEngine::playbackWorker() {
    const int periodSize = 1024;
    const int sampleRate = 44100;
    
    std::vector<float> floatBuffer(periodSize * 2, 0.0f); // 立体声浮点缓冲
    std::vector<short> shortBuffer(periodSize * 2, 0);    // ALSA 播放所需的 16-bit 缓冲
    std::vector<float> mono(periodSize, 0.0f);            // 用于算频谱的单声道混音缓冲

    // 对应紫、蓝、青、绿、黄、橙、红的 7 个代表性频率 (Hz)
    std::vector<int> targetFreqs = {60, 150, 400, 1000, 2400, 6000, 12000};
    std::vector<float> spectrum(7, 0.0f);

    while (running) {
        if (source) {
            // 步骤 A: 向解码器索要一帧纯净的音频数据
            source->onProcessAudio(floatBuffer, periodSize);

            // 步骤 B: 将左右声道混合成单声道，给灯带算频谱用
            for (int i = 0; i < periodSize; ++i) {
                mono[i] = (floatBuffer[i * 2] + floatBuffer[i * 2 + 1]) * 0.5f;
            }

            // 步骤 C: 用 Goertzel 算法揪出这 7 个频率的能量
            float maxMag = 0.0f;
            for (size_t i = 0; i < targetFreqs.size(); ++i) {
                spectrum[i] = goertzelMagnitude(periodSize, targetFreqs[i], sampleRate, mono.data());
                if (spectrum[i] > maxMag) maxMag = spectrum[i];
            }

            // 步骤 D: 数据归一化 (防爆音、防白屏)
            if (maxMag > 1e-6f) {
                for (float& v : spectrum) v /= maxMag;
            } else {
                std::fill(spectrum.begin(), spectrum.end(), 0.0f);
            }

            // 🌟 步骤 E: 把算好的灯效数据秒传给硬件管家！(瞬间完成，绝不阻塞)
            if (hwController) {
                hwController->updateLighting(spectrum);
            }

            // 步骤 F: 把浮点音频转回 16-bit PCM，准备发给蓝牙耳机播放
            for (int i = 0; i < periodSize * 2; ++i) {
                float val = floatBuffer[i];
                if (val > 1.0f) val = 1.0f;
                if (val < -1.0f) val = -1.0f;
                shortBuffer[i] = static_cast<short>(val * 32767.0f);
            }

            // 步骤 G: 真正的物理播放 (写入底层 ALSA)
            int err = snd_pcm_writei(pcmHandle, shortBuffer.data(), periodSize);
            if (err < 0) {
                recoverFromError(err);
            }
        } else {
            // 如果还没准备好音乐，休眠一下防 CPU 空转
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
        }
    }
}
