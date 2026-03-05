#pragma once
#include <atomic>
#include <functional>
#include <mutex>
#include <string>
#include <vector>

class Player {
public:
    using Callback = std::function<void()>;

    bool init();
    void shutdown();

    void setPlaylist(std::vector<std::string> files);
    const std::vector<std::string>& playlist() const;

    bool playIndex(int idx);
    void play();
    void pause();
    void toggle();
    void stop();
    void next();
    void prev();
    void seekSeconds(double sec);

    void setVolume(float v01);       // 0..1
    float volume() const;

    bool isPlaying() const;
    int  currentIndex() const;

    double positionSeconds() const;
    double durationSeconds() const;

    // For LEDs: Current output "energy" estimate (0..1)
    float level01() const;

    // UI refresh callbacks (track switching/state changes)
    void setOnStateChanged(Callback cb);

    std::string currentTitle() const;

private:
    struct Impl;
    Impl* impl_{nullptr};
};
