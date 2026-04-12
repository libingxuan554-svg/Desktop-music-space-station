/**
 * @file HardwareController.cpp
 * @brief Implementation of the hardware management interface for audio,
 *        LED strip control, and basic platform hardware services.
 *
 * This source file implements the HardwareController class. The controller
 * provides a thread-safe wrapper around:
 * - miniaudio engine initialisation and shutdown
 * - LED strip initialisation and spectrum-based updates
 * - ALSA mixer based hardware volume control
 * - placeholder hooks for other hardware modules such as the touchscreen
 *
 * Concurrency note:
 * All public operations in this implementation acquire the same mutex in
 * order to protect shared hardware state and avoid race conditions between
 * initialisation, shutdown, lighting updates, and mixer access.
 */

// This macro must be defined in exactly one CPP file to implement miniaudio
#define MINIAUDIO_IMPLEMENTATION

#include "HardwareController.hpp"

#include <alsa/asoundlib.h>
#include <iostream>

/**
 * @brief Construct a HardwareController with default internal state.
 *
 * Initial state:
 * - hardware system not initialised
 * - LED strip not initialised
 * - cached hardware volume range set to 0-100 until queried from ALSA
 */
HardwareController::HardwareController()

 : m_isInitialized(false),

 m_ledStripInitialized(false),

 m_minVolume(0),

 m_maxVolume(100) {

}

/**
 * @brief Destroy the HardwareController and release managed hardware resources.
 *
 * The destructor delegates cleanup to shutdown() so that the audio engine
 * and LED strip are safely released before the object is destroyed.
 */
HardwareController::~HardwareController() {

 shutdown();

}

/**
 * @brief Initialise the core hardware subsystem.
 * @return true if the controller is already initialised or miniaudio is
 *         initialised successfully; false otherwise.
 *
 * This function currently initialises the miniaudio engine, which is used
 * by the project as the main audio backend. The operation is protected by
 * a mutex so that hardware startup is safe when accessed from multiple
 * threads.
 */
bool HardwareController::initialize() {

 std::lock_guard<std::mutex> lock(m_hardwareMutex);

 if (m_isInitialized) return true;

 // Initialise the miniaudio engine used by the hardware controller.
 ma_result result = ma_engine_init(NULL, &m_audioEngine);

 if (result != MA_SUCCESS) {
 std::cerr << "Failed to initialize miniaudio engine." << std::endl;

 return false;

 }

 m_isInitialized = true;

 std::cout << "Hardware Init Success." << std::endl;

 return true;

}

/**
 * @brief Shut down all initialised hardware managed by this controller.
 *
 * Shutdown order:
 * 1. Shut down the LED strip if it was initialised.
 * 2. Uninitialise the miniaudio engine if it was initialised.
 *
 * The function is safe to call multiple times because internal state flags
 * are checked before each shutdown step.
 */
void HardwareController::shutdown() {

 std::lock_guard<std::mutex> lock(m_hardwareMutex);

 if (m_ledStripInitialized) {

 m_ledStrip.shutdown();

 m_ledStripInitialized = false;

 }

 if (m_isInitialized) {

 ma_engine_uninit(&m_audioEngine);

 m_isInitialized = false;

 }

}

/**
 * @brief Initialise the LED strip controller.
 * @return true if the LED strip is already initialised or initialises
 *         successfully; false otherwise.
 *
 * This function delegates low-level LED setup to the LedStripController
 * instance and records the resulting initialisation state locally.
 */
bool HardwareController::initLedStrip() {
 std::lock_guard<std::mutex> lock(m_hardwareMutex);

 if (m_ledStripInitialized) return true;

 if (!m_ledStrip.initialize()) {

 std::cerr << "Failed to initialize LED strip." << std::endl;

 return false;

 }

 m_ledStripInitialized = true;

 return true;

}

/**
 * @brief Update LED lighting from an audio spectrum vector.
 * @param spectrum Audio spectrum data used to generate the lighting effect.
 *
 * The function forwards the spectrum data to the LED strip controller.
 * If the LED strip has not been initialised yet, the update request is
 * ignored safely.
 */
void HardwareController::updateLighting(const std::vector<float>& spectrum) {

 std::lock_guard<std::mutex> lock(m_hardwareMutex);

 if (!m_ledStripInitialized) return;

 m_ledStrip.updateFromSpectrum(spectrum);

}

/**
 * @brief Clear the current LED lighting output.
 *
 * This requests the LED strip controller to turn off or reset its current
 * light state. If the LED strip has not been initialised, the request is
 * ignored safely.
 */
void HardwareController::clearLighting() {

 std::lock_guard<std::mutex> lock(m_hardwareMutex);

 if (!m_ledStripInitialized) return;

 m_ledStrip.clear();

}

/**
 * @brief Set the system playback volume through the ALSA mixer.
 * @param volume_percent Requested volume percentage in the range 0 to 100.
 * @return true if the ALSA mixer volume is updated successfully; false on
 *         any mixer setup or write failure.
 *
 * Behaviour:
 * - Input values below 0 are clamped to 0.
 * - Input values above 100 are clamped to 100.
 * - The percentage value is mapped into the hardware playback volume range
 *   reported by the ALSA mixer element named "Master".
 *
 * Implementation note:
 * The mixer handle and simple element identifiers are opened and released
 * within this function so that no external ALSA state is kept alive between
 * calls.
 */
bool HardwareController::setHardwareVolume(long volume_percent) {

 std::lock_guard<std::mutex> lock(m_hardwareMutex);

 if (volume_percent < 0) volume_percent = 0;

 if (volume_percent > 100) volume_percent = 100;

 snd_mixer_t* handle = nullptr;

 snd_mixer_selem_id_t* sid = nullptr;

 const char* card = "default";
 const char* selem_name = "Master";

 if (snd_mixer_open(&handle, 0) < 0) return false;

 if (snd_mixer_attach(handle, card) < 0) {

 snd_mixer_close(handle);

 return false;

 }

 if (snd_mixer_selem_register(handle, NULL, NULL) < 0) {

 snd_mixer_close(handle);

 return false;

 }

 if (snd_mixer_load(handle) < 0) {

 snd_mixer_close(handle);

 return false;

 }

 snd_mixer_selem_id_malloc(&sid);

 if (!sid) {

 snd_mixer_close(handle);

 return false;

 }

 snd_mixer_selem_id_set_index(sid, 0);
 snd_mixer_selem_id_set_name(sid, selem_name);

 snd_mixer_elem_t* elem = snd_mixer_find_selem(handle, sid);

 if (!elem) {

 std::cerr << "Cannot find ALSA mixer element." << std::endl;

 snd_mixer_selem_id_free(sid);

 snd_mixer_close(handle);

 return false;

 }

 snd_mixer_selem_get_playback_volume_range(elem, &m_minVolume, &m_maxVolume);

 long hardware_volume =

 m_minVolume + (volume_percent * (m_maxVolume - m_minVolume) / 100);
 int result = snd_mixer_selem_set_playback_volume_all(elem, hardware_volume);

 snd_mixer_selem_id_free(sid);

 snd_mixer_close(handle);

 return result >= 0;

}

/**
 * @brief Read the current system playback volume from the ALSA mixer.
 * @return Current playback volume as a percentage in the range 0 to 100,
 *         or -1 if the mixer cannot be accessed.
 *
 * The function reads the current playback level from the "Master" mixer
 * element, converts it from the hardware volume range into a percentage,
 * and clamps the result to the interval [0, 100].
 */
long HardwareController::getHardwareVolume() {

 std::lock_guard<std::mutex> lock(m_hardwareMutex);

 snd_mixer_t* handle = nullptr;

 snd_mixer_selem_id_t* sid = nullptr;

 const char* card = "default";

 const char* selem_name = "Master";

 if (snd_mixer_open(&handle, 0) < 0) return -1;

 if (snd_mixer_attach(handle, card) < 0) {
 snd_mixer_close(handle);

 return -1;

 }

 if (snd_mixer_selem_register(handle, NULL, NULL) < 0) {

 snd_mixer_close(handle);

 return -1;

 }

 if (snd_mixer_load(handle) < 0) {

 snd_mixer_close(handle);

 return -1;

 }

 snd_mixer_selem_id_malloc(&sid);

 if (!sid) {

 snd_mixer_close(handle);

 return -1;

 }

 snd_mixer_selem_id_set_index(sid, 0);

 snd_mixer_selem_id_set_name(sid, selem_name);

 snd_mixer_elem_t* elem = snd_mixer_find_selem(handle, sid);

 if (!elem) {
 snd_mixer_selem_id_free(sid);

 snd_mixer_close(handle);

 return -1;

 }

 long min = 0, max = 0, current = 0;

 snd_mixer_selem_get_playback_volume_range(elem, &min, &max);

 snd_mixer_selem_get_playback_volume(elem, SND_MIXER_SCHN_FRONT_LEFT, &current);

 snd_mixer_selem_id_free(sid);

 snd_mixer_close(handle);

 if (max == min) return 0;

 long percent = (current - min) * 100 / (max - min);

 if (percent < 0) percent = 0;

 if (percent > 100) percent = 100;

 return percent;

}

/**
 * @brief Initialise the touchscreen subsystem.
 * @return true currently, because this function is still a placeholder.
 *
 * Current status:
 * This is a stub hook reserved for future touchscreen driver integration.
 * It does not yet perform any real hardware initialisation.
 */
bool HardwareController::initTouchScreen() {

 // Placeholder for touchscreen driver initialization

 return true;

}
