#pragma once
#include "AlsaOut.h"
#include "Mp3DecodeAll.h"
#include <atomic>
#include <condition_variable>
#include <mutex>
#include <thread>
#include <string>

class PlayerCore {
public:
    PlayerCore() = default;
    ~PlayerCore();

    bool load(const std::string& mp3Path); // decode + open ALSA (match format)
    void play();
    void pause();   // prefer snd_pcm_pause(1)
    void resume();  // prefer snd_pcm_pause(0)
    void stop();

    void setVolume(float v); // 0..1

private:
    void threadFunc();
    static int16_t clamp16(int32_t x);

    std::mutex m_;
    std::condition_variable cv_;

    std::thread th_;
    std::atomic<bool> running_{false};
    std::atomic<bool> paused_{false};

    bool hwPauseSupported_ = false;

    float volume_ = 1.0f;

    AlsaOut out_;
    DecodedPcm pcm_;
    std::string loadedPath_;

    // playback cursor in frames
    int64_t framePos_ = 0;
};
