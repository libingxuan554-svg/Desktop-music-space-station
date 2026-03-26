#include "../include/WavDecoder.hpp"
#include <iostream>
#include <string>
#include <fcntl.h>    // For Linux OS-level file open()
#include <unistd.h>   // For Linux OS-level read(), close(), lseek()

WavDecoder::WavDecoder() : fileDescriptor(-1), isInitialized(false), dataStartPosition(44) {
    std::cout << "[WavDecoder] Pure C++ OS-Level Decoder Created." << std::endl;
}

WavDecoder::~WavDecoder() {
    // RAII principle: automatically close file to prevent memory leaks
    closeFile();
    std::cout << "[WavDecoder] Decoder Destroyed safely." << std::endl;
}

bool WavDecoder::openFile(const std::string& filepath) {
    if (isInitialized) {
        closeFile();
    }

    // 1. Use low-level POSIX API for maximum I/O performance
    fileDescriptor = open(filepath.c_str(), O_RDONLY);
    if (fileDescriptor < 0) {
        std::cerr << "[WavDecoder] ERROR: Could not open file: " << filepath << std::endl;
        return false;
    }

    // 2. 读取基础头部 (只读前 36 字节，包含 RIFF, WAVE 和 16字节的 fmt 块)
    uint8_t header[36];
    if (read(fileDescriptor, header, 36) != 36) {
        std::cerr << "[WavDecoder] ERROR: File is too small or invalid." << std::endl;
        closeFile();
        return false;
    }

    // 3. 解析基础格式信息 (从 fmt 块提取，Little-Endian 移位)
    format.numChannels = header[22] | (header[23] << 8);
    format.sampleRate  = header[24] | (header[25] << 8) | (header[26] << 16) | (header[27] << 24);
    format.bitDepth    = header[34] | (header[35] << 8);

    // 4. 智能雷达：扫描寻找 "data" 块 (自动跳过 LIST 等非标准干扰信息)
    char chunkId[5] = {0}; 
    uint32_t chunkSize = 0;
    bool foundData = false;

    while (true) {
        // 读取 4 个字母的标识符 (如 "data", "LIST")
        if (read(fileDescriptor, chunkId, 4) != 4) break; 
        
        // 读取这个块的字节大小
        uint8_t sizeBuffer[4];
        if (read(fileDescriptor, sizeBuffer, 4) != 4) break;
        chunkSize = sizeBuffer[0] | (sizeBuffer[1] << 8) | (sizeBuffer[2] << 16) | (sizeBuffer[3] << 24);

        if (std::string(chunkId, 4) == "data") {
            format.dataSize = chunkSize;
