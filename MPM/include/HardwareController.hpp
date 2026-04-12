#ifndef HARDWARE_CONTROLLER_HPP
#define HARDWARE_CONTROLLER_HPP

#include <mutex>
#include <vector>
#include <thread>
#include <atomic>
#include <condition_variable>
#include "LedStripController.hpp"

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
    void updateLighting(const std::vector<float>& spectrum);
    void clearLighting();

    //灯带的频谱数据直接暴露给UI模块
    std::vector<float> getCurrentSpectrum() {
        std::lock_guard<std::mutex> lock(m_spectrumMutex); // 保证线程安全
        return m_currentSpectrum;
    }

private:
    void ledWorker(); // 🌟 新增：独立灯效线程函数

    std::mutex m_hardwareMutex;
    bool m_isInitialized;
    bool m_ledStripInitialized;
    LedStripController m_ledStrip;
    long m_minVolume;
    long m_maxVolume;

    // 🌟 线程与信箱控制变量
    std::thread m_ledThread;
    std::atomic<bool> m_ledRunning;
    std::mutex m_spectrumMutex;
    std::condition_variable m_ledCV;
    std::vector<float> m_currentSpectrum;
};

#endif
