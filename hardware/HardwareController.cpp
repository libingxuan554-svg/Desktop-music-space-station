// This macro must be defined in exactly one CPP file to implement miniaudio
#define MINIAUDIO_IMPLEMENTATION

#include "HardwareController.hpp"

#include <alsa/asoundlib.h>
#include <iostream>

HardwareController::HardwareController()
    : m_isInitialized(false),
      m_ledStripInitialized(false),
      m_minVolume(0),
      m_maxVolume(100) {
}

HardwareController::~HardwareController() {
    shutdown();
}

bool HardwareController::initialize() {
    std::lock_guard<std::mutex> lock(m_hardwareMutex);

    if (m_isInitialized) return true;

    // 1. Initialize Miniaudio
    ma_result result = ma_engine_init(NULL, &m_audioEngine);
    if (result != MA_SUCCESS) {
        std::cerr << "Failed to initialize miniaudio engine." << std::endl;
        return false;
    }

    m_isInitialized = true;
    std::cout << "Hardware Init Success." << std::endl;
    return true;
}

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

void HardwareController::updateLighting(const std::vector<float>& spectrum) {
    std::lock_guard<std::mutex> lock(m_hardwareMutex);

    if (!m_ledStripInitialized) return;
    m_ledStrip.updateFromSpectrum(spectrum);
}

void HardwareController::clearLighting() {
    std::lock_guard<std::mutex> lock(m_hardwareMutex);

    if (!m_ledStripInitialized) return;
    m_ledStrip.clear();
}

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

bool HardwareController::initTouchScreen() {
    // Placeholder for touchscreen driver initialization
    return true;
}
