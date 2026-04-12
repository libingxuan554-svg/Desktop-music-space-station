#pragma once

#include <vector>
#include <cstdint>

/**
 * @class LedStripController
 * @brief Low-level controller for an SPI-based LED strip.
 *
 * This class provides the LED output interface used by the project.
 * It is responsible for:
 * - opening and configuring the Linux SPI device
 * - converting application colour data into LED strip frames
 * - updating the strip from spectrum data
 * - clearing and shutting down the LED strip safely
 */
class LedStripController {

public:

 /**
  * @brief Initialise the SPI device used to drive the LED strip.
  * @param device Path to the Linux SPI device, for example /dev/spidev0.0.
  * @param speed SPI clock speed in Hz.
  * @return true if the SPI device is opened and configured successfully;
  *         false otherwise.
  */
 bool initialize(const char* device = "/dev/spidev0.0", uint32_t speed = 3200000);

 /**
  * @brief Update LED output from one frame of spectrum data.
  * @param spectrum Audio spectrum magnitudes used to generate the lighting
  *        effect.
  *
  * The spectrum is mapped to LED brightness and colour distribution across
  * the strip.
  */
 void updateFromSpectrum(const std::vector<float>& spectrum);

 /**
  * @brief Clear the LED strip output.
  *
  * This turns off all LEDs currently controlled by the strip controller.
  */
 void clear();

 /**
  * @brief Shut down the LED strip controller.
  *
  * This should release any hardware resources associated with the SPI device
  * and leave the strip in a safe cleared state.
  */
 void shutdown();

private:

 /**
  * @brief Send one complete LED frame to the strip.
  * @param colors LED colour values encoded as 24-bit GRB entries.
  */
 void sendLeds(const std::vector<uint32_t>& colors);

 /**
  * @brief Pack separate colour channels into one 24-bit GRB value.
  * @param g Green channel intensity.
  * @param r Red channel intensity.
  * @param b Blue channel intensity.
  * @return Packed 24-bit colour value in GRB byte order.
  */
 static uint32_t makeGRB(uint8_t g, uint8_t r, uint8_t b);

 /**
  * @brief File descriptor for the opened SPI device.
  */
 int fd = -1;

 /**
  * @brief Smoothed spectrum values used to reduce abrupt visual changes.
  */
 std::vector<float> m_smoothedSpectrum;

 /**
  * @brief Total number of LEDs controlled on the strip.
  */
 static constexpr int LED_COUNT = 60;

 /**
  * @brief Number of trailing reset bytes appended after an LED frame.
  */
 static constexpr int RESET_BYTES = 500;

};
