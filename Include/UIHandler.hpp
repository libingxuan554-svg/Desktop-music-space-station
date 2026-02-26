#ifndef UIHANDLER_HPP
#define UIHANDLER_HPP

#include <vector>
#include <string>

/**
 * @brief Abstract Interface for UI Event Handling
 * Focus: Decoupling UI from Audio/Hardware threads using callbacks.
 */
class UIHandler {
public:
    virtual ~UIHandler() = default;

    // --- HARDWARE DRIVEN EVENTS (Inbound) ---

    // Triggered by Infrared Sensor thread to switch UI layouts
    // isMusicActive: true for Player View, false for Clock/Weather View
    virtual void onSystemModeChanged(bool isMusicActive) = 0;

    // Triggered by Audio Analysis thread (FFT result)
    // frequencyBins: Vector of amplitudes for visualizer bars (RT-Deadline critical)
    virtual void onVisualizerDataReady(const std::vector<float>& frequencyBins) = 0;

    // Triggered by Temperature/Weather thread
    virtual void onEnvironmentUpdate(float currentTemp, const std::string& weatherCondition) = 0;

    // --- USER INTERACTION CALLBACKS (Outbound) ---
    
    // Callbacks provided to UI to control the Media branch
    struct InteractionSignals {
        void (*requestPlayPause)();
        void (*requestVolumeAdjust)(int level);
    };
};

#endif
