#include "WavDecoder.hpp"
#include <iostream>
#include <cstring>
#include <fcntl.h>    // POSIX 文件控制
#include <unistd.h>   // POSIX 读取

// 1. 构造函数：基础变量的安全初始化 (Failsafe)
WavDecoder::WavDecoder() : fileDescriptor(-1), currentBytePosition(0) {
    // format 会自动调用你在 AudioFormat.hpp 里写的默认构造函数，全置为 0
}

// 析构函数：确保文件被安全关闭，防止文件描述符泄漏
WavDecoder::~WavDecoder() {
    closeFile();
}

// 2. 外部调用的入口：打开文件并触发初始化步骤
bool WavDecoder::openFile(const std::string& filepath) {
    // 确保如果之前打开了别的文件，先关掉
    if (fileDescriptor >= 0) {
        closeFile();
    }

    // 使用 Linux 底层 POSIX 'open' 而不是 std::ifstream，性能更高，更贴近 Real-time 要求
    fileDescriptor = open(filepath.c_str(), O_RDONLY);
    if (fileDescriptor < 0) {
        std::cerr << "[WavDecoder] Error: Cannot open file " << filepath << std::endl;
        return false;
    }

    // 【核心初始化步骤】：解析头部并填充 AudioFormat
    if (!parseHeader()) {
        std::cerr << "[WavDecoder] Error: Invalid WAV format or corrupted header." << std::endl;
        closeFile();
        return false;
    }

    // 初始化成功！将系统文件指针安全地移动到真正音乐数据（PCM）的开头
    currentBytePosition = format.dataOffset;
    lseek(fileDescriptor, currentBytePosition, SEEK_SET);
    
    std::cout << "[WavDecoder] Successfully initialized! Sample Rate: " 
              << format.sampleRate << "Hz, Channels: " << format.numChannels << std::endl;
              
    return true;
}

void WavDecoder::closeFile() {
    if (fileDescriptor >= 0) {
        close(fileDescriptor);
        fileDescriptor = -1;
    }
}

// 3. 私有的初始化逻辑：解析 44 字节的 WAV 头
bool WavDecoder::parseHeader() {
    uint8_t header[44];
    
    // 读取前 44 个字节
    if (read(fileDescriptor, header, 44) != 44) {
        return false;
    }

    // 防御性编程 (Fault Detection)：检查是不是真正的 WAV 文件
    if (std::memcmp(header, "RIFF", 4) != 0 || std::memcmp(header + 8, "WAVE", 4) != 0) {
        return false;
    }

    // 【解析数据并填充到你的 AudioFormat 结构体中】
    // (WAV 是小端序 Little-Endian，所以需要用位移操作拼接字节)
    format.numChannels = header[22] | (header[23] << 8);
    format.sampleRate  = header[24] | (header[25] << 8) | (header[26] << 16) | (header[27] << 24);
    format.bitDepth    = header[34] | (header[35] << 8);
    
    // 假设是标准 WAV，数据块通常从第 44 字节开始
    format.dataOffset  = 44; 
    format.dataSize    = header[40] | (header[41] << 8) | (header[42] << 16) | (header[43] << 24);

    // 调用你在 AudioFormat 里写的校验函数，确保解析出的数据合法！
    return format.isValid();
}

// 获取已解析的格式信息（供一号位 ALSA 引擎使用）
AudioFormat WavDecoder::getFormat() const {
    return format;
}
