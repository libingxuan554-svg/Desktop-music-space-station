#ifndef AUDIO_ENGINE_HPP
#define AUDIO_ENGINE_HPP

#include <alsa/asoundlib.h>
#include <thread>
#include <atomic>
#include <vector>
#include "AudioSource.hpp"

// 提前告诉编译器：外面有一个叫 HardwareController 的类
class HardwareController; 

class AudioEngine {
public:
    // 🌟 核心修复：这里现在允许接收两个参数了！（第二个参数默认是 nullptr，防止报错）
    AudioEngine(AudioSource* src, HardwareController* hw = nullptr);
    ~AudioEngine();

    bool init(const char* device, unsigned int sampleRate);
    void start();
    void stop();

private:
    void playbackWorker();
    void recoverFromError(int err);

    AudioSource* source;
    HardwareController* hwController; // 存放硬件管家的名片
    snd_pcm_t* pcmHandle;
    std::thread playbackThread;
    std::atomic<bool> running;
};

#endif
