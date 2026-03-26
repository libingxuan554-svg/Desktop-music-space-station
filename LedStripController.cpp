#include "LedStripController.hpp"
#include <unistd.h>
#include <vector>
#include <cstdint>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/spi/spidev.h>

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

void LedStripController::updateFromPeak(float peak) {
    if (peak < 0.0f) peak = 0.0f;
    if (peak > 1.0f) peak = 1.0f;

    int numOn = static_cast<int>(peak * LED_COUNT);

    std::vector<uint32_t> leds(LED_COUNT, 0);

    for (int i = 0; i < numOn; ++i) {
        if (i < 20) leds[i] = 0x440000;
        else if (i < 40) leds[i] = 0x444400;
        else leds[i] = 0x004400;
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
