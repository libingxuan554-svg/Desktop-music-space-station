#pragma once 
#include <vector> 
#include <cstdint> 

// ==========================================
/**
 * @class LedStripController
 * @brief  Responsible for directly controlling WS2812B physical LED strips via SPI interface.
 * @note [Architecture / SOLID]:
 * -  | Follows SRP: Completely decouples low-level SPI communication protocol from high-level audio resolution.
 */
class LedStripController {
public:
// ==========================================
/**
 * @brief  | Initialize the SPI device node
 * @param[in] device SPI /dev/spidev0.0 | SPI device path, defaults to Raspberry Pi's /dev/spidev0.0
 * @param[in] speed SPI SPI communication baud rate, defaults to 3.2MHz (adapted for WS2812B timing)
 * @return  Whether the hardware was successfully opened and configured
 * @note [Real-Time Constraints]:
 * -  Determinism: Generates file I/O overhead only during system boot; absolutely no open/ioctl blocking operations during runtime.
 */
    bool initialize(const char* device = "/dev/spidev0.0", uint32_t speed = 3200000); //  | Mount hardware interface
// ==========================================

// ==========================================
/**
 * @brief Receive spectrum data and convert it to LED color frames for refreshing
 * @param[in] spectrum  Normalized spectrum energy array resolved by the audio engine
 * @note [Real-Time Constraints]:
 * // [REAL-TIME COMPLIANCE]:
 * //  Core color space rendering algorithms use pure memory O(N) traversal calculations, ensuring extreme-speed completion within the 33.3ms (30FPS) budget.
 */
    void updateFromSpectrum(const std::vector<float>& spectrum); //  Drive physical rhythm of the LED strip
// ==========================================

// ==========================================
/**
 * @brief  Clear the state of all LED beads (full black)
 */
    void clear(); // Extinguish all physical light points
// ==========================================

// ==========================================
/**
 * @brief  Safely close the SPI device and extinguish the LED strip
 */
    void shutdown(); // Release hardware handle resources
// ==========================================

private:
// ==========================================
/**
 * @brief Low-level dotting transmission function: Send GRB array to physical device via SPI
 * @param[in] colors  Array containing 24-bit color information of all LED beads
 * @note [Real-Time Constraints]:
 * // [REAL-TIME COMPLIANCE]:
 * //  Directly utilizes Linux kernel's SPI hardware buffer mechanism for burst transmission of memory.
 * //  Completely avoids 100% CPU full-load polling blocking caused by traditional GPIO Bit-banging!
 */
    void sendLeds(const std::vector<uint32_t>& colors); //  Execute low-level SPI message delivery
// ==========================================

// ==========================================
/**
 * @brief  Pack independent channel colors into WS2812B exclusive GRB format
 */
    static uint32_t makeGRB(uint8_t g, uint8_t r, uint8_t b); // Color space conversion inline helper
// ==========================================

    int fd = -1; //  Stores the file descriptor for the SPI device
    std::vector<float> m_smoothedSpectrum; //  Caches previous frame's band state, used for exponential smoothing decay visual effects

    static constexpr int LED_COUNT = 100; //  Number of real LED beads contained in the physical light strip
    static constexpr int RESET_BYTES = 500; //  Reset silence bytes at the end of data packet required by WS2812B protocol
};
