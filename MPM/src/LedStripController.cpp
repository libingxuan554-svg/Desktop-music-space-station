#include "../include/LedStripController.hpp"
#include <vector>
#include <cmath>
#include <algorithm>
#include <cstdint>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/spi/spidev.h>
#include <iostream>
#include <chrono>

// ==========================================
// 1. 底层硬件驱动 (保持你完美的 8MHz 时序)
// ==========================================

bool LedStripController::initialize(const char* device, uint32_t speed) {
    fd = open(device, O_RDWR);
    if (fd < 0) return false;
    uint8_t mode = 0;
    ioctl(fd, SPI_IOC_WR_MODE, &mode);
    return true;
}

void LedStripController::shutdown() {
    if (fd >= 0) {
        clear();
        close(fd);
        fd = -1;
    }
}

void LedStripController::clear() {
    std::vector<uint32_t> empty(LED_COUNT, 0);
    sendLeds(empty);
}

uint32_t LedStripController::makeGRB(uint8_t g, uint8_t r, uint8_t b) {
    return (g << 16) | (r << 8) | b;
}

void LedStripController::sendLeds(const std::vector<uint32_t>& colors) {
    if (fd < 0) return;
    
    std::vector<uint8_t> spiData;
    spiData.reserve(colors.size() * 24 + RESET_BYTES);
    
    for (uint32_t c : colors) {
        for (int i = 23; i >= 0; i--) {
            if ((c >> i) & 1) spiData.push_back(0b11111100); 
            else spiData.push_back(0b11100000);              
        }
    }
    for (int i = 0; i < RESET_BYTES; i++) spiData.push_back(0);

    struct spi_ioc_transfer tr = {0};
    tr.tx_buf = (unsigned long)spiData.data();
    tr.len = spiData.size();
    tr.speed_hz = 8000000;
    tr.bits_per_word = 8;
    ioctl(fd, SPI_IOC_MESSAGE(1), &tr);
}

// ==========================================
// 2. 视觉算法：专业级起批与拖尾特效
// ==========================================

void LedStripController::updateFromSpectrum(const std::vector<float>& spectrum) {
    static auto lastUpdate = std::chrono::steady_clock::now();
    auto now = std::chrono::steady_clock::now();
    // 锁定最高 30 帧，保证视觉连贯性
    if (std::chrono::duration_cast<std::chrono::milliseconds>(now - lastUpdate).count() < 33) {
        return; 
    }
    lastUpdate = now;

    const int COLOR_GROUPS = 7;
    
    // 🌟 视觉优化 1：全局降低 70% 的绝对亮度，拒绝刺眼！(数值最高才 45)
    const std::vector<uint32_t> palette = {
        makeGRB(0,   30,  30),   // 紫
        makeGRB(0,   0,   45),   // 蓝
        makeGRB(35,  0,   35),   // 青
        makeGRB(35,  0,   0),    // 绿
        makeGRB(35,  35,  0),    // 黄
        makeGRB(20,  45,  0),    // 橙
        makeGRB(0,   45,  0)     // 红
    };

    std::vector<uint32_t> leds(LED_COUNT, 0);
    std::vector<float> safeSpectrum = spectrum;
    
    if (safeSpectrum.empty()) {
        std::fill(leds.begin(), leds.end(), palette[0]);
        sendLeds(leds);
        return;
    }

    for (float& v : safeSpectrum) {
        if (std::isnan(v) || std::isinf(v) || v < 0.0f) v = 0.0f;
    }

    std::vector<float> groups(COLOR_GROUPS, 0.0f);
    const int bandCount = static_cast<int>(safeSpectrum.size());

    for (int i = 0; i < bandCount; ++i) {
        float v = safeSpectrum[i];
        int group = (i * COLOR_GROUPS) / bandCount;
        if (group >= COLOR_GROUPS) group = COLOR_GROUPS - 1;
        groups[group] += v;
    }

    float total = 0.0f;
    for (float v : groups) total += v;

    if (total < 1e-6f) {
        sendLeds(leds);
        return;
    }
    for (float& v : groups) v /= total;

    if (m_smoothedSpectrum.size() != COLOR_GROUPS) {
        m_smoothedSpectrum.assign(COLOR_GROUPS, 0.0f);
    }

    // 🌟 视觉优化 2：非对称平滑算法 (Attack & Decay)
    for (int i = 0; i < COLOR_GROUPS; ++i) {
        if (groups[i] > m_smoothedSpectrum[i]) {
            // 鼓点袭来：80% 权重接受新数据，瞬间爆发！
            m_smoothedSpectrum[i] = 0.20f * m_smoothedSpectrum[i] + 0.80f * groups[i];
        } else {
            // 声音减弱：保留 85% 的老数据，制造缓慢变暗的呼吸拖尾感！
            m_smoothedSpectrum[i] = 0.85f * m_smoothedSpectrum[i] + 0.15f * groups[i];
        }
    }

    const float EDGE_BOOST = 1.8f;
    m_smoothedSpectrum[0] *= EDGE_BOOST;                 
    m_smoothedSpectrum[COLOR_GROUPS - 1] *= EDGE_BOOST; 

    float smoothTotal = 0.0f;
    for (float v : m_smoothedSpectrum) smoothTotal += v;
    if (smoothTotal < 1e-6f) smoothTotal = 1.0f;
    for (float& v : m_smoothedSpectrum) v /= smoothTotal;

    const int half = LED_COUNT / 2;
    std::vector<int> counts(COLOR_GROUPS, 0);
    std::vector<float> exact(COLOR_GROUPS, 0.0f);

    int used = 0;
    for (int i = 0; i < COLOR_GROUPS; ++i) {
        exact[i] = m_smoothedSpectrum[i] * half;
        counts[i] = static_cast<int>(std::floor(exact[i]));
        used += counts[i];
    }

    while (used < half) {
        int best = 0;
        float bestFrac = -1.0f;
        for (int i = 0; i < COLOR_GROUPS; ++i) {
            float frac = exact[i] - static_cast<float>(counts[i]);
            if (frac > bestFrac) {
                bestFrac = frac;
                best = i;
            }
        }
        counts[best]++;
        used++;
    }

    const int MIN_EDGE = 3;
    if (counts[0] < MIN_EDGE) {
        counts[0] += (MIN_EDGE - counts[0]);
        used += (MIN_EDGE - counts[0]);
    }
    if (counts[COLOR_GROUPS - 1] < MIN_EDGE) {
        counts[COLOR_GROUPS - 1] += (MIN_EDGE - counts[COLOR_GROUPS - 1]);
        used += (MIN_EDGE - counts[COLOR_GROUPS - 1]);
    }

    while (used > half) {
        int best = -1;
        int bestCount = -1;
        for (int i = 1; i < COLOR_GROUPS - 1; ++i) {
            if (counts[i] > bestCount) {
                bestCount = counts[i];
                best = i;
            }
        }
        if (best >= 0 && counts[best] > 0) {
            counts[best]--;
            used--;
        } else {
            break;
        }
    }

    int pos = 0;
    for (int c = 0; c < COLOR_GROUPS; ++c) {
        for (int j = 0; j < counts[c] && pos < half; ++j) {
            leds[pos++] = palette[c];
        }
    }
    while (pos < half) leds[pos++] = palette[COLOR_GROUPS - 1];
    for (int i = 0; i < half; ++i) leds[LED_COUNT - 1 - i] = leds[i];

    sendLeds(leds);
}
