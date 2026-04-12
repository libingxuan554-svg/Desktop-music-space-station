/**
 * @file LedStripController.cpp
 * @brief Implementation of SPI-based LED strip control and spectrum-driven
 *        lighting updates.
 *
 * This file implements the low-level LED strip controller used by the project.
 * It is responsible for:
 * - opening and configuring the Linux SPI device
 * - converting logical LED colour values into the SPI waveform encoding
 * - generating LED frames from spectrum data
 * - clearing and shutting down the LED strip safely
 */

#include "LedStripController.hpp"

#include <unistd.h>

#include <vector>

#include <cstdint>

#include <fcntl.h>

#include <sys/ioctl.h>

#include <linux/spi/spidev.h>
#include <algorithm>

/**
 * @brief Open and configure the SPI device used by the LED strip.
 * @param device Path to the Linux SPI device, for example /dev/spidev0.0.
 * @param speed SPI clock speed in Hz.
 * @return true if the SPI device is opened and configured successfully;
 *         false otherwise.
 *
 * This function configures the controller for SPI mode 0 and stores the file
 * descriptor for later LED frame transmission.
 */
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

/**
 * @brief Pack one LED colour into 24-bit GRB format.
 * @param g Green channel intensity.
 * @param r Red channel intensity.
 * @param b Blue channel intensity.
 * @return 24-bit colour value in GRB byte order.
 *
 * The LED strip expects colour data in GRB order rather than RGB.
 */
uint32_t LedStripController::makeGRB(uint8_t g, uint8_t r, uint8_t b) {

 return (static_cast<uint32_t>(g) << 16) |
 (static_cast<uint32_t>(r) << 8) |

 static_cast<uint32_t>(b);

}

/**
 * @brief Send a complete LED frame to the strip over SPI.
 * @param colors Vector of LED colours encoded as 24-bit GRB values.
 *
 * Each colour bit is converted into the corresponding SPI byte pattern
 * required by the LED strip protocol. A reset tail is appended at the end
 * of the frame so the strip latches the new data.
 */
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

/**
 * @brief Update LED output from one frame of spectrum data.
 * @param spectrum Magnitude values for frequency bands in the current frame.
 *
 * Processing steps:
 * - if the spectrum is empty, clear the strip
 * - find the maximum band magnitude for frame-level normalisation
 * - smooth each band to reduce rapid visual flicker
 * - map each band to a section of the LED strip
 * - assign colour by frequency region and brightness by band intensity
 */
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

 // Low-frequency bands: red to orange
 if (t < 0.20f) return makeGRB(0, v, 0); // red

 if (t < 0.40f) return makeGRB(v / 5, v, 0); // orange

 // Mid-frequency bands: yellow to green-cyan
 if (t < 0.60f) return makeGRB(v, v, 0); // yellow
 if (t < 0.80f) return makeGRB(v, 0, v / 4); // green-cyan

 // High-frequency bands: blue to purple
 return makeGRB(v / 6, v / 6, v); // blue-purple

 };

 int start = 0;

 for (int i = 0; i < bandCount; ++i) {

 float normalized = spectrum[i] / frameMax;

 if (normalized < 0.0f) normalized = 0.0f;

 if (normalized > 1.0f) normalized = 1.0f;

 // Apply smoothing to reduce unstable frame-to-frame jumps
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

/**
 * @brief Turn off all LEDs on the strip.
 *
 * This creates an all-zero LED frame and sends it to the strip.
 */
void LedStripController::clear() {

 std::vector<uint32_t> leds(LED_COUNT, 0);

 sendLeds(leds);

}

/**
 * @brief Clear the LED strip and release the SPI device.
 *
 * The strip is first turned off so that shutdown leaves the hardware in a
 * safe visual state. The SPI file descriptor is then closed if it is valid.
 */
void LedStripController::shutdown() {

 clear();

 if (fd >= 0) {

 close(fd);

 fd = -1;

 }

}
