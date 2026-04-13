#include "../include/WavDecoder.hpp"
#include <iostream>
#include <algorithm>
#include <cstring>

WavDecoder::WavDecoder() : totalDataSize(0), sampleRate(44100), channels(2) {}

WavDecoder::~WavDecoder() {
    if (file.is_open()) file.close();
}

/**
 * @brief Opens the WAV file and safely parses the RIFF/PCM header.
 * @note [Real-Time Constraints]:
 * - Blocking I/O: Contains inherently non-deterministic disk I/O operations. MUST be executed asynchronously or during the pre-load phase, strictly before the real-time ALSA thread begins.
 */
bool WavDecoder::open(const std::string& path) {
    if (file.is_open()) {
        file.close();
        file.clear(); 
    }
    
    file.open(path, std::ios::binary);
    if (!file.is_open()) return false;

    char header[44];
    file.read(header, 44);

    
    channels = *reinterpret_cast<short*>(header + 22);
    sampleRate = *reinterpret_cast<int*>(header + 24);
    
    // Recalculate the data size instead of trusting Subchunk2Size in the header
    file.seekg(0, std::ios::end);
    uint32_t fileSize = static_cast<uint32_t>(file.tellg());
    totalDataSize = (fileSize > 44) ? (fileSize - 44) : 0;
    
    
    file.clear();
    file.seekg(44, std::ios::beg);

    std::cout << "[WavDecoder] Success! SR: " << sampleRate << "Hz, CH: " << channels 
              << ", DataSize: " << totalDataSize << " bytes" << std::endl;
    return true;
}

/**
 * @brief Calculates and mutates the file read pointer based on playback progress.
 * @note [Real-Time Constraints]:
 * - O(1) Pointer Mutation: Fast `seekg` operation.
 * - Memory Alignment: Strictly enforces 4-byte block alignment (16-bit stereo) to categorically prevent audio tearing, static noise, or channel phase inversion during seeking.
 */
void WavDecoder::seekTo(float progress) {
    if (!file.is_open() || totalDataSize == 0) return;

    progress = std::clamp(progress, 0.0f, 1.0f);
    
    //  Calculate the target byte offset relative to the data section (after 44-byte header)
    uint32_t targetOffset = static_cast<uint32_t>(totalDataSize * progress);
    
    //  Align to 4 bytes (16-bit stereo = 4 bytes per frame)
    targetOffset = (targetOffset / 4) * 4;
    
    uint32_t finalPos = 44 + targetOffset;

    file.clear();
    file.seekg(finalPos, std::ios::beg);
    std::cout << "⏩ Seek: " << (progress * 100.0f) << "% (byte position: " << finalPos << ")" << std::endl;
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

/**
 * @brief Reads raw PCM bytes and normalizes them to interleaved floats (Producer).
 * @note [Real-Time Constraints]:
 * - I/O Jitter Absorption: Since disk reads involve unpredictable latency, this function operates as a background producer. It continuously pre-fetches data into the Lock-Free RingBuffer, completely absorbing disk jitter and shielding the hardware playback thread.
 */
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
