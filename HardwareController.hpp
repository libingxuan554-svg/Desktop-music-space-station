#ifndef HARDWARE_CONTROLLER_HPP
#define HARDWARE_CONTROLLER_HPP

#include <mutex>
#include <string>

#include "miniaudio.h"
#include "LedStripController.hpp"

class HardwareController {
public:
    HardwareController();
    ~HardwareController();

    // Responsible for the program's initialization process
    bool initialize();
    void shutdown();

    // ALSA Mixer hardware volume control
    bool setHardwareVolume(long volume_percent); // 0 to 100
    long getHardwareVolume();

    // Other hardware init hooks
    bool initTouchScreen();

    // LED strip control
    bool initLedStrip();
    void updateLighting(const std::vector<float>& spectrum);
    void clearLighting();

private:
    // For multi-thread synchronization to avoid deadlocks
    std::mutex m_hardwareMutex;

    bool m_isInitialized;
    bool m_ledStripInitialized;

    // miniaudio engine instance
    ma_engine m_audioEngine;

    // LED strip controller
    LedStripController m_ledStrip;

    // Internal ALSA helper variables
    long m_minVolume;
    long m_maxVolume;
};

#endif // HARDWARE_CONTROLLER_HPP
