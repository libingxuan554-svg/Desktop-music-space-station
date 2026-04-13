#ifndef HARDWARE_CONTROLLER_HPP
#define HARDWARE_CONTROLLER_HPP

#include <mutex>
#include <vector>
#include <thread>
#include <atomic>
#include <condition_variable>
#include "LedStripController.hpp"

/**
 * @class HardwareController
 * @brief Facade for hardware peripherals (Volume, Touch, LEDs).
 * @note [Architecture / SOLID]:
 * - Single Responsibility Principle (SRP): Isolates physical hardware control from audio decoding and UI logic.
 * - Facade Pattern: Provides a unified, thread-safe interface for disparate lower-level hardware subsystems.
 */
class HardwareController {
public:
    HardwareController();
    ~HardwareController();

    bool initialize();
    void shutdown();

    bool setHardwareVolume(long volume_percent);
    long getHardwareVolume();
    bool initTouchScreen();

    bool initLedStrip();
    /**
 * @brief Dispatches spectrum data to the asynchronous LED worker thread.
 * @param[in] spectrum The analyzed audio spectrum data.
 * @note [Real-Time Constraints]:
 * - Non-Blocking: Uses a condition variable (`m_ledCV`) to wake the worker. Returns immediately (O(1)) without blocking on slow hardware SPI writes.
 */
    void updateLighting(const std::vector<float>& spectrum);
    void clearLighting();

    /**
 * @brief Safely retrieves the current spectrum for UI rendering.
 * @return A copy of the latest spectrum data.
 * @note [Real-Time Constraints]:
 * - Thread Safety: Protected by `m_spectrumMutex` to prevent data races between the fast audio analyzer and the UI renderer.
 */
    std::vector<float> getCurrentSpectrum() {
        std::lock_guard<std::mutex> lock(m_spectrumMutex); // Ensure thread safety
        return m_currentSpectrum;
    }

private:
/**
 * @brief Dedicated background daemon for SPI LED rendering.
 * @note [Real-Time Constraints]:
 * - Zero-Polling: Thread sleeps natively via `m_ledCV.wait()` until a state change occurs, ensuring 0% CPU idling.
 */
    void ledWorker(); // New: dedicated LED effect worker thread

    std::mutex m_hardwareMutex;
    bool m_isInitialized;
    bool m_ledStripInitialized;
    LedStripController m_ledStrip;
    long m_minVolume;
    long m_maxVolume;

    // Thread and mailbox control variables
    std::thread m_ledThread;
    std::atomic<bool> m_ledRunning;
    std::mutex m_spectrumMutex;
    std::condition_variable m_ledCV;
    std::vector<float> m_currentSpectrum;
};

#endif
