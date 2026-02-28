#ifndef AUDIOFORMAT_HPP
#define AUDIOFORMAT_HPP

#include <cstdint>

/**
 * @brief 音频格式元数据结构 (Audio Metadata Structure)
 * * 用于封装从音频文件头 (如 WAV header) 中解析出的关键物理参数。
 * 这些数据将作为桥梁，传递给底层的 ALSA 音频引擎进行硬件参数配置，
 * 同时也是实现 O(1) 确定性进度跳转 (Deterministic Seek) 的核心数学依据。
 */
struct AudioFormat {
    uint32_t sampleRate;  // 采样率 (例如: 44100 Hz, 48000 Hz)
    uint16_t numChannels; // 声道数 (例如: 1 代表单声道, 2 代表立体声)
    uint16_t bitDepth;    // 采样位数/位深 (例如: 16-bit)
    uint32_t dataSize;    // 纯 PCM 数据的总字节数 (不包含文件头的大小)
    uint32_t dataOffset;  // PCM 数据在原始文件中的起始字节偏移量 (跳过 Header)

    // 默认构造函数：确保实例化时所有内存被清零，防止读取到随机垃圾内存 (Failsafe)
    AudioFormat() 
        : sampleRate(0), numChannels(0), bitDepth(0), dataSize(0), dataOffset(0) {}

    // 辅助函数：校验解析出的格式是否合法 (Fault detection)
    bool isValid() const {
        return (sampleRate > 0 && numChannels > 0 && bitDepth > 0 && dataSize > 0 && dataOffset > 0);
    }
};

#endif // AUDIOFORMAT_HPP
