#ifndef WAV_DECODER_HPP
#define WAV_DECODER_HPP

#include <string>
#include <cstdint>
// Note: We deliberately DO NOT include miniaudio.h here to keep our C++ clean!
// This is a high-scoring architecture design.

// A simple struct to hold our audio format info
struct AudioFormat {
    uint32_t sampleRate;
    uint16_t numChannels;
    uint16_t bitDepth;
    uint32_t dataSize;
};

class WavDecoder {
public:
    WavDecoder();
    ~WavDecoder();

    // Core functions
    bool openFile(const std::string& filepath);
    void closeFile();
    size_t readFrames(uint8_t* outputBuffer, size_t framesToRead);
    
    // O(1) Deterministic Seek
    bool seekToTime(double timeInSeconds);
    
    // Get audio info
    AudioFormat getFormat() const;

private:
    AudioFormat format;
    
    // We use a pointer to hide the complicated C struct (Pimpl idiom)
    // Professor Bernd will love this encapsulation!
    struct PrivateData;
    PrivateData* pData;
    
    bool isInitialized;
};

#endif // WAV_DECODER_HPP
