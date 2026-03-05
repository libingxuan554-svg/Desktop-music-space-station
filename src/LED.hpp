#pragma once
#include <atomic>
#include <thread>

class LED {
public:
    bool start(int gpioPin, int ledCount);
    void stop();

    // 0..1
    void setLevel(float v);

private:
    std::atomic<bool> running{false};
    std::atomic<float> level{0.0f};
    std::thread th;

    int gpio = 18;
    int count = 30;
};
