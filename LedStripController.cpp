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
    ioctl(fd, SPI_IOC_WR_MODE, &mode);
    ioctl(fd, SPI_IOC_WR_MAX_SPEED_HZ, &speed);

    return true;
}

void LedStripController::sendLeds(const std::vector<uint32_t>& colors) {
    std::vector<uint8_t> spi_data;

    for (uint32_t grb : colors) {
        for (int i = 23; i >= 0; i--) {
            spi_data.push_back(((grb >> i) & 1) ? 0xFE : 0x80);
        }
    }

    for (int i = 0; i < 500; i++) {
        spi_data.push_back(0x00);
    }

    write(fd, spi_data.data(), spi_data.size());
}
