#ifndef WAV_DECODER_HPP
#define WAV_DECODER_HPP

#include <string>
#include <vector>
#include <fstream>
#include <cstdint>

class WavDecoder {
public:
    WavDecoder();
    ~WavDecoder();

    bool open(const std::string& path);
    void onProcessAudio(std::vector<float>& buffer, uint32_t numFrames);
    void seekTo(float progress);
    
    bool seekToTime(double seconds);
    double getCurrentPosition();   // 注意：去掉了 const，修复 tellg 报错
    double getTotalDuration() const;

private:
    std::ifstream file;
    uint32_t totalDataSize;
    int sampleRate;
    int channels;
};

#endif // <--- 必须有这一行！
