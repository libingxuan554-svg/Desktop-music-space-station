#ifndef HARDWARE_CONTROLLER_HPP
#define HARDWARE_CONTROLLER_HPP

#include <mutex>

#include <string>
#include "miniaudio.h"
#include "LedStripController.hpp"

/**
 * @class HardwareController
 * @brief Central controller for platform-level hardware services.
 *
 * This class provides a single interface for:
 * - core hardware initialisation and shutdown
 * - ALSA mixer based hardware volume control
 * - touchscreen initialisation hooks
 * - LED strip initialisation and lighting updates
 *
 * Thread safety:
 * Access to shared hardware state is protected internally by a mutex.
 */
class HardwareController {

public:

 /**
  * @brief Construct a hardware controller.
  */
 HardwareController();

 /**
  * @brief Destroy the hardware controller and release managed resources.
  */
 ~HardwareController();

 /**
  * @brief Perform the program's hardware initialisation process.
  * @return true if initialisation succeeds or the controller is already
  *         initialised; false otherwise.
  */
 bool initialize();

 /**
  * @brief Shut down all hardware subsystems managed by this controller.
  */
 void shutdown();

 /**
  * @brief Set the hardware playback volume through the ALSA mixer.
  * @param volume_percent Requested volume percentage in the range 0 to 100.
  * @return true if the volume is set successfully; false otherwise.
  */
 bool setHardwareVolume(long volume_percent); // 0 to 100

 /**
  * @brief Get the current hardware playback volume from the ALSA mixer.
  * @return Current playback volume as a percentage.
  */
 long getHardwareVolume();

 /**
  * @brief Initialise other hardware hooks such as the touchscreen.
  * @return true if touchscreen initialisation succeeds.
  */
 bool initTouchScreen();

 /**
  * @brief Initialise the LED strip controller.
  * @return true if LED strip initialisation succeeds; false otherwise.
  */
 bool initLedStrip();

 /**
  * @brief Update LED lighting from a spectrum vector.
  * @param spectrum Audio spectrum data used to drive the lighting effect.
  */
 void updateLighting(const std::vector<float>& spectrum);

 /**
  * @brief Clear the current LED lighting output.
  */
 void clearLighting();

private:

 /**
  * @brief Mutex used for multi-thread synchronisation of shared hardware state.
  */
 std::mutex m_hardwareMutex;

 /**
  * @brief Indicates whether the core hardware subsystem has been initialised.
  */
 bool m_isInitialized;

 /**
  * @brief Indicates whether the LED strip controller has been initialised.
  */
 bool m_ledStripInitialized;

 /**
  * @brief miniaudio engine instance used for audio-related hardware services.
  */
 ma_engine m_audioEngine;

 /**
  * @brief LED strip controller used for visual output.
  */
 LedStripController m_ledStrip;

 /**
  * @brief Minimum ALSA mixer playback volume supported by the hardware.
  */
 long m_minVolume;

 /**
  * @brief Maximum ALSA mixer playback volume supported by the hardware.
  */
 long m_maxVolume;

};
#endif // HARDWARE_CONTROLLER_HPP
