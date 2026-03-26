void LedStripController::sendLeds(const std::vector<uint32_t>& colors)
    std::vector<uint8_t> spi_data;
    for (uint32_t grb : colors) {
        for (int i = 23; i >= 0; i--) {
            spi_data.push_back(((grb >> i) & 1) ? 0xFE : 0x80);
        }
    }
    for(int i = 0; i < 500; i++) spi_data.push_back(0x00);
    write(fd, spi_data.data(), spi_data.size());
}
