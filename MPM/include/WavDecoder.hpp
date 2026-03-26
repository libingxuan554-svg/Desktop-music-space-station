#ifndef WAV_DECODER_HPP
#define WAV_DECODER_HPP

#include <string>
#include <cstdint>
#include <cstddef>

// Struct to hold extracted WAV header information
struct AudioFormat {
    uint32_t sampleRate;
    uint16_t numChannels;
    uint16_t bitDepth;
    uint32_t dataSize; // Total bytes of raw audio data
};

class WavDecoder {
public:
    WavDecoder();
    ~WavDecoder();

    // Core lifecycle functions
    bool openFile(const std::string& filepath);
    void closeFile();
    
    // Read raw audio bytes into the RingBuffer
    size_t readFrames(uint8_t* outputBuffer, size_t framesToRead);
    
    // O(1) Time complexity seek using manual physical memory calculation
    bool seekToTime(double timeInSeconds);
    
    AudioFormat getFormat() const;
// --- 新增：对接团队 PlaybackStatus 接口 ---
    int32_t getTotalDuration() const;
    int32_t getCurrentPosition() const;
private:
    int fileDescriptor;       // Linux POSIX file handle
    AudioFormat format;
    bool isInitialized;
    size_t dataStartPosition; // Where the actual music starts (usually byte 44)
};

#endif // WAV_DECODER_HPP
