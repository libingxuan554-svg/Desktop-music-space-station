#include "PlayerCore.h"
#include <algorithm>
#include <cmath>
#include <iostream>

PlayerCore::~PlayerCore() {
    stop();
}

int16_t PlayerCore::clamp16(int32_t x) {
    if (x > 32767) return 32767;
    if (x < -32768) return -32768;
    return (int16_t)x;
}

bool PlayerCore::load(const std::string& mp3Path) {
    stop(); // stop any current playback

    DecodedPcm decoded;
    if (!decodeMp3All(mp3Path, decoded)) return false;

    if (decoded.channels <= 0 || decoded.sampleRate <= 0 || decoded.samples.empty()) {
        std::cerr << "invalid decoded pcm\n";
        return false;
    }

    // Open ALSA to match decoded format
    if (!out_.open((unsigned int)decoded.sampleRate, decoded.channels)) return false;

    pcm_ = std::move(decoded);
    loadedPath_ = mp3Path;
    framePos_ = 0;

    // test hardware pause support once (toggle off->on->off safely)
    hwPauseSupported_ = out_.pause(false); // try resume first
    if (hwPauseSupported_) {
        // some devices return 0 for resume even if pause isn't supported;
        // do a real pause probe:
        hwPauseSupported_ = out_.pause(true);
        if (hwPauseSupported_) hwPauseSupported_ = out_.pause(false);
    }

    std::cout << "Loaded: " << loadedPath_
              << " rate=" << pcm_.sampleRate
              << " ch=" << pcm_.channels
              << " hwPause=" << (hwPauseSupported_ ? "yes" : "no") << "\n";
    return true;
}

void PlayerCore::play() {
    if (pcm_.samples.empty()) {
        std::cerr << "play(): no track loaded\n";
        return;
    }

    if (running_) return;

    running_ = true;
    paused_ = false;
    th_ = std::thread(&PlayerCore::threadFunc, this);
}

void PlayerCore::pause() {
    if (!running_) return;
    paused_ = true;

    if (hwPauseSupported_) {
        out_.pause(true); // hardware pause
    }
    // If not supported, thread will block via condition_variable
}

void PlayerCore::resume() {
    if (!running_) return;

    if (hwPauseSupported_) {
        out_.pause(false); // hardware resume
    }

    paused_ = false;
    cv_.notify_all();
}

void PlayerCore::stop() {
    if (!running_) {
        out_.close();
        pcm_.samples.clear();
        loadedPath_.clear();
        framePos_ = 0;
        return;
    }

    running_ = false;
    paused_ = false;
    cv_.notify_all();

    if (th_.joinable()) th_.join();

    out_.close();
    pcm_.samples.clear();
    loadedPath_.clear();
    framePos_ = 0;
}

void PlayerCore::setVolume(float v) {
    std::lock_guard<std::mutex> lk(m_);
    volume_ = std::clamp(v, 0.0f, 1.0f);
}

void PlayerCore::threadFunc() {
    const int ch = pcm_.channels;
    const int64_t totalFrames = (int64_t)pcm_.samples.size() / ch;

    // chunk size matches ALSA period for stability
    const snd_pcm_sframes_t chunkFrames = (snd_pcm_sframes_t)out_.periodFrames();

    // scratch buffer for volume-scaled chunk (avoid modifying original samples permanently)
    std::vector<int16_t> scratch((size_t)chunkFrames * ch);

    while (running_ && framePos_ < totalFrames) {
        // If no hardware pause, block thread during pause
        if (paused_ && !hwPauseSupported_) {
            std::unique_lock<std::mutex> lk(m_);
            cv_.wait(lk, [&]{ return !running_ || !paused_; });
            if (!running_) break;
        }

        // If hardware pause is supported, ALSA will stop consuming; we can keep thread light:
        if (paused_ && hwPauseSupported_) {
            // Sleep-like wait: avoid busy loop
            std::unique_lock<std::mutex> lk(m_);
            cv_.wait_for(lk, std::chrono::milliseconds(10), [&]{ return !running_ || !paused_; });
            continue;
        }

        const int64_t framesLeft = totalFrames - framePos_;
        const snd_pcm_sframes_t framesThis = (snd_pcm_sframes_t)std::min<int64_t>(chunkFrames, framesLeft);

        const int16_t* src = pcm_.samples.data() + framePos_ * ch;

        float vol;
        {
            std::lock_guard<std::mutex> lk(m_);
            vol = volume_;
        }

        if (vol >= 0.999f) {
            // no scaling, write directly
            out_.writeFrames(src, framesThis);
        } else {
            // scale into scratch then write
            const int64_t samplesToScale = (int64_t)framesThis * ch;
            for (int64_t i = 0; i < samplesToScale; i++) {
                int32_t v = (int32_t)std::lround((float)src[i] * vol);
                scratch[(size_t)i] = clamp16(v);
            }
            out_.writeFrames(scratch.data(), framesThis);
        }

        framePos_ += framesThis;
    }

    // auto stop when finished
    running_ = false;
}
