#pragma once
#include <functional>
#include <string>
#include <vector>

class Player {
public:
    using Callback = std::function<void()>;

    Player();
    ~Player();

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

    void setVolume(float v01);   // 0..1
    float volume() const;

    bool isPlaying() const;
    int  currentIndex() const;

    double positionSeconds() const;
    double durationSeconds() const;

    // For LED/UI
    float level01() const;

    // 主线程/UI线程周期调用
    void update();

    void setOnStateChanged(Callback cb);
    std::string currentTitle() const;

private:
    struct Impl;
    Impl* impl_;
};
