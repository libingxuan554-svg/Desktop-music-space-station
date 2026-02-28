#ifndef HARDWARE_CONTROLLER_HPP
#define HARDWARE_CONTROLLER_HPP

#include <mutex>
#include <string>

// Include miniaudio header
#include "miniaudio.h"

class HardwareController {
public:
    HardwareController();
    ~HardwareController();

    // Responsible for the program's initialization process
    bool initialize();
    void shutdown();

    // ALSA Mixer hardware volume control (Core assessment point)
    bool setHardwareVolume(long volume_percent); // 0 to 100
    long getHardwareVolume();

    // Interfaces can be reserved for other initializations like touch screen drivers
    bool initTouchScreen();

private:
    // For multi-thread synchronization to avoid deadlocks
    std::mutex m_hardwareMutex; 
    bool m_isInitialized;

    // miniaudio engine instance
    ma_engine m_audioEngine; 

    // Internal ALSA helper variables
    long m_minVolume;
    long m_maxVolume;
};

#endif // HARDWARE_CONTROLLER_HPP
