#include "LedStripController.hpp"
#include <unistd.h>
#include <vector>
#include <cstdint>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/spi/spidev.h>
#include <algorithm>

bool LedStripController::initialize(const char* device, uint32_t speed) {
    fd = open(device, O_RDWR);
    if (fd < 0) return false;

    uint8_t mode = SPI_MODE_0;

    if (ioctl(fd, SPI_IOC_WR_MODE, &mode) < 0) {
        close(fd);
        fd = -1;
        return false;
    }

    if (ioctl(fd, SPI_IOC_WR_MAX_SPEED_HZ, &speed) < 0) {
        close(fd);
        fd = -1;
        return false;
    }

    return true;
}

uint32_t LedStripController::makeGRB(uint8_t g, uint8_t r, uint8_t b) {
    return (static_cast<uint32_t>(g) << 16) |
           (static_cast<uint32_t>(r) << 8)  |
           static_cast<uint32_t>(b);
}

void LedStripController::sendLeds(const std::vector<uint32_t>& colors) {
    if (fd < 0) return;

    std::vector<uint8_t> spi_data;
    spi_data.reserve(colors.size() * 24 + RESET_BYTES);

    for (uint32_t grb : colors) {
        for (int i = 23; i >= 0; --i) {
            spi_data.push_back(((grb >> i) & 1) ? 0xFE : 0x80);
        }
    }

    for (int i = 0; i < RESET_BYTES; ++i) {
        spi_data.push_back(0x00);
    }

    write(fd, spi_data.data(), spi_data.size());
}

void LedStripController::updateFromSpectrum(const std::vector<float>& spectrum) {
    if (spectrum.empty()) {
        clear();
        return;
    }

    float frameMax = *std::max_element(spectrum.begin(), spectrum.end());
    if (frameMax < 1e-6f) {
        clear();
        return;
    }

    if (m_smoothedSpectrum.size() != spectrum.size()) {
        m_smoothedSpectrum.assign(spectrum.size(), 0.0f);
    }

    const int bandCount = static_cast<int>(spectrum.size());
    const int ledsPerBand = LED_COUNT / bandCount;
    const int extra = LED_COUNT % bandCount;

    std::vector<uint32_t> leds(LED_COUNT, 0);

    auto colorForBand = [&](int band, float intensity) -> uint32_t {
        uint8_t v = static_cast<uint8_t>(20 + intensity * 100);

        float t = (bandCount == 1) ? 0.0f
                                   : static_cast<float>(band) / static_cast<float>(bandCount - 1);

        // 低频 -> 红 / 橙
        if (t < 0.20f) return makeGRB(0, v, 0);          // red
        if (t < 0.40f) return makeGRB(v / 5, v, 0);      // orange

        // 中频 -> 黄 / 绿
        if (t < 0.60f) return makeGRB(v, v, 0);          // yellow
        if (t < 0.80f) return makeGRB(v, 0, v / 4);      // green-cyan

        // 高频 -> 蓝 / 紫
        return makeGRB(v / 6, v / 6, v);                 // blue-purple
    };

    int start = 0;

    for (int i = 0; i < bandCount; ++i) {
        float normalized = spectrum[i] / frameMax;
        if (normalized < 0.0f) normalized = 0.0f;
        if (normalized > 1.0f) normalized = 1.0f;

        // 平滑一下，避免乱跳
        if (normalized > m_smoothedSpectrum[i]) {
            m_smoothedSpectrum[i] = 0.45f * m_smoothedSpectrum[i] + 0.55f * normalized;
        } else {
            m_smoothedSpectrum[i] = 0.80f * m_smoothedSpectrum[i] + 0.20f * normalized;
        }

        int segmentLen = ledsPerBand + (i < extra ? 1 : 0);
        int litCount = static_cast<int>(m_smoothedSpectrum[i] * segmentLen + 0.5f);

        uint32_t color = colorForBand(i, m_smoothedSpectrum[i]);

        for (int j = 0; j < litCount && j < segmentLen; ++j) {
            leds[start + j] = color;
        }

        start += segmentLen;
    }

    sendLeds(leds);
}

void LedStripController::clear() {
    std::vector<uint32_t> leds(LED_COUNT, 0);
    sendLeds(leds);
}

void LedStripController::shutdown() {
    clear();

    if (fd >= 0) {
        close(fd);
        fd = -1;
    }
}
