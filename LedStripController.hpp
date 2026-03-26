#pragma once
#include <vector>
#include <cstdint>

class LedStripController {
public:
    bool initialize(const char* device = "/dev/spidev0.0", uint32_t speed = 3200000);
    void updateFromPeak(float peak);
    void clear();
    void shutdown();

private:
    void sendLeds(const std::vector<uint32_t>& colors);
    int fd = -1;
    static constexpr int LED_COUNT = 60;
};
