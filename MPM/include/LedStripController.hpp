#pragma once
#include <vector>
#include <cstdint>

class LedStripController {
public:
    bool initialize(const char* device = "/dev/spidev0.0", uint32_t speed = 3200000);
    void updateFromSpectrum(const std::vector<float>& spectrum);
    void clear();
    void shutdown();

private:
    void sendLeds(const std::vector<uint32_t>& colors);
    static uint32_t makeGRB(uint8_t g, uint8_t r, uint8_t b);

    int fd = -1;
    std::vector<float> m_smoothedSpectrum;

    static constexpr int LED_COUNT = 100;
    static constexpr int RESET_BYTES = 500;
};
