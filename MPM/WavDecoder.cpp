#include "../include/WavDecoder.hpp"
#include <iostream>
#include <algorithm>
#include <cstring>

WavDecoder::WavDecoder() : totalDataSize(0), sampleRate(44100), channels(2) {}

WavDecoder::~WavDecoder() {
    if (file.is_open()) file.close();
}

bool WavDecoder::open(const std::string& path) {
    if (file.is_open()) {
        file.close();
        file.clear(); 
    }
    
    file.open(path, std::ios::binary);
    if (!file.is_open()) return false;

    char header[44];
    file.read(header, 44);

    // 解析基础信息
    channels = *reinterpret_cast<short*>(header + 22);
    sampleRate = *reinterpret_cast<int*>(header + 24);
    
    // 强制重新计算大小，不信任 Header 里的 Subchunk2Size
    file.seekg(0, std::ios::end);
    uint32_t fileSize = static_cast<uint32_t>(file.tellg());
    totalDataSize = (fileSize > 44) ? (fileSize - 44) : 0;
    
    // 回到数据起始位置
    file.clear();
    file.seekg(44, std::ios::beg);

    std::cout << "[WavDecoder] Success! SR: " << sampleRate << "Hz, CH: " << channels 
              << ", DataSize: " << totalDataSize << " bytes" << std::endl;
    return true;
}

void WavDecoder::seekTo(float progress) {
    if (!file.is_open() || totalDataSize == 0) return;

    progress = std::clamp(progress, 0.0f, 1.0f);
    
    // 计算目标字节：必须相对于数据区(44字节后)进行偏移
    uint32_t targetOffset = static_cast<uint32_t>(totalDataSize * progress);
    
    // 🌟 关键：4字节对齐 (16bit立体声 = 4 bytes/frame)
    targetOffset = (targetOffset / 4) * 4;
    
    uint32_t finalPos = 44 + targetOffset;

    file.clear();
    file.seekg(finalPos, std::ios::beg);
    std::cout << "⏩ 进度跳转: " << (progress * 100.0f) << "% (字节位置: " << finalPos << ")" << std::endl;
}

double WavDecoder::getCurrentPosition() {
    if (!file.is_open() || sampleRate == 0 || channels == 0) return 0.0;
    long pos = (long)file.tellg() - 44;
    if (pos < 0) pos = 0;
    return (double)pos / (sampleRate * channels * 2);
}

double WavDecoder::getTotalDuration() const {
    if (sampleRate == 0 || channels == 0) return 0.0;
    return (double)totalDataSize / (sampleRate * channels * 2);
}

bool WavDecoder::seekToTime(double seconds) {
    double duration = getTotalDuration();
    if (duration <= 0) return false;
    seekTo(static_cast<float>(seconds / duration));
    return true;
}

void WavDecoder::onProcessAudio(std::vector<float>& buffer, uint32_t numFrames) {
    if (!file.is_open() || file.eof()) {
        std::fill(buffer.begin(), buffer.end(), 0.0f);
        return;
    }
    size_t samplesToRead = numFrames * channels;
    std::vector<short> rawData(samplesToRead);
    file.read(reinterpret_cast<char*>(rawData.data()), samplesToRead * sizeof(short));
    size_t samplesRead = file.gcount() / sizeof(short);
    for (size_t i = 0; i < samplesRead; ++i) buffer[i] = rawData[i] / 32768.0f;
    if (samplesRead < samplesToRead) std::fill(buffer.begin() + samplesRead, buffer.end(), 0.0f);
}
