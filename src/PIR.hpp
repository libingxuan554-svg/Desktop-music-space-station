#pragma once
#include <atomic>
#include <functional>
#include <string>
#include <thread>

class PIR {
public:
    using Callback = std::function<void()>;

    bool start(const std::string& chipName, int line, Callback onMotion);
    void stop();

private:
    std::atomic<bool> running{false};
    std::thread th;
};
