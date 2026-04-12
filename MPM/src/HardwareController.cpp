#include "../include/HardwareController.hpp"
#include <alsa/asoundlib.h>
#include <iostream>
#include <chrono>

HardwareController::HardwareController()
    : m_isInitialized(false), m_ledStripInitialized(false),
      m_minVolume(0), m_maxVolume(100), m_ledRunning(false) {}

HardwareController::~HardwareController() { shutdown(); }

bool HardwareController::initialize() {
    std::lock_guard<std::mutex> lock(m_hardwareMutex);
    if (m_isInitialized) return true;
    m_isInitialized = true;
    return true;
}

bool HardwareController::initLedStrip() {
    std::lock_guard<std::mutex> lock(m_hardwareMutex);
    if (m_ledStripInitialized) return true;

    if (!m_ledStrip.initialize()) {
        std::cerr << "LED SPI 初始化失败" << std::endl;
        return false;
    }
    m_ledStripInitialized = true;

    // Core idea: start an independent LED refresh worker thread
    m_ledRunning = true;
    m_ledThread = std::thread(&HardwareController::ledWorker, this);
    return true;
}

void HardwareController::shutdown() {
    if (m_ledRunning) {
        m_ledRunning = false;
        if (m_ledThread.joinable()) m_ledThread.join();
    }
    std::lock_guard<std::mutex> lock(m_hardwareMutex);
    if (m_ledStripInitialized) {
        m_ledStrip.shutdown();
        m_ledStripInitialized = false;
    }
    m_isInitialized = false;
}

void HardwareController::updateLighting(const std::vector<float>& spectrum) {
    {
    //The audio thread only pushes data into the "mailbox" and returns immediately without blocking
    std::lock_guard<std::mutex> lock(m_spectrumMutex);
    m_currentSpectrum = spectrum;
    }
    m_ledCV.notify_one(); // Wake up the sleeping LED worker
}

void HardwareController::ledWorker() {
    while (m_ledRunning) {
        std::vector<float> specCopy;
        {
            std::unique_lock<std::mutex> lock(m_spectrumMutex);
            // Fully suspend the thread. It will only be woken up by notify_one
            m_ledCV.wait(lock, [this] { 
                return !m_currentSpectrum.empty() || !m_ledRunning; 
            });

            if (!m_ledRunning) break; // or when the system is preparing to exit

            specCopy = m_currentSpectrum;
        }

        if (m_ledStripInitialized && !specCopy.empty()) {
            m_ledStrip.updateFromSpectrum(specCopy);
        }

    }
}

void HardwareController::clearLighting() {
    std::lock_guard<std::mutex> lock(m_spectrumMutex);
    m_currentSpectrum.clear();
}
