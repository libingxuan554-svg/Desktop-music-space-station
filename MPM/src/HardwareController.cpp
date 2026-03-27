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

    // 🌟 核心：启动独立的 LED 刷新工人！
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
    // 🌟 音频线程只负责把数据“丢进信箱”，瞬间返回，绝不阻塞！
    std::lock_guard<std::mutex> lock(m_spectrumMutex);
    m_currentSpectrum = spectrum;
}

void HardwareController::ledWorker() {
    // 🌟 独立工人：自己按照 30FPS 的节奏去信箱拿数据并刷新硬件
    while (m_ledRunning) {
        std::vector<float> specCopy;
        {
            std::lock_guard<std::mutex> lock(m_spectrumMutex);
            specCopy = m_currentSpectrum;
        }

        if (m_ledStripInitialized && !specCopy.empty()) {
            m_ledStrip.updateFromSpectrum(specCopy);
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(33));
    }
}

void HardwareController::clearLighting() {
    std::lock_guard<std::mutex> lock(m_spectrumMutex);
    m_currentSpectrum.clear();
}

// =====================================
// 下面的 ALSA 控制代码保留不动...
