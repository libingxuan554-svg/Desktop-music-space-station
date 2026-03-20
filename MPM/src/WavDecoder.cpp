#include "../include/WavDecoder.hpp"
#include <iostream>
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

    // 1. Use low-level POSIX API for maximum I/O performance (No heavy streams)
    fileDescriptor = open(filepath.c_str(), O_RDONLY);
    if (fileDescriptor < 0) {
        std::cerr << "[WavDecoder] ERROR: Could not open file: " << filepath << std::endl;
        return false;
    }

    // 2. Standard WAV PCM header is 44 bytes
    uint8_t header[44];
    if (read(fileDescriptor, header, 44) != 44) {
        std::cerr << "[WavDecoder] ERROR: File is too small or not a valid WAV." << std::endl;
        closeFile();
        return false;
    }

    // 3. Extract format info using bitwise shift (Little-Endian parsing)
    format.numChannels = header[22] | (header[23] << 8);
    format.sampleRate  = header[24] | (header[25] << 8) | (header[26] << 16) | (header[27] << 24);
    format.bitDepth    = header[34] | (header[35] << 8);
    format.dataSize    = header[40] | (header[41] << 8) | (header[42] << 16) | (header[43] << 24);
    
    // Music data starts immediately after the 44-byte header
    dataStartPosition = 44; 
    isInitialized = true;

    std::cout << "[WavDecoder] Success! SR: " << format.sampleRate 
              << "Hz, CH: " << format.numChannels 
              << ", Depth: " << format.bitDepth << "bit." << std::endl;
    return true;
}

void WavDecoder::closeFile() {
    if (fileDescriptor >= 0) {
        close(fileDescriptor);
        fileDescriptor = -1;
        isInitialized = false;
        std::cout << "[WavDecoder] File closed." << std::endl;
    }
}

size_t WavDecoder::readFrames(uint8_t* outputBuffer, size_t framesToRead) {
    if (!isInitialized || fileDescriptor < 0) return 0;
    
    // Calculate how many bytes we need
    size_t bytesPerFrame = format.numChannels * (format.bitDepth / 8);
    size_t bytesToRead = framesToRead * bytesPerFrame;
    
    // Read directly from the OS file system
    ssize_t bytesRead = read(fileDescriptor, outputBuffer, bytesToRead);
    
    if (bytesRead <= 0) {
        return 0; // End of file or error
    }
    
    // Return the actual number of frames read
    return static_cast<size_t>(bytesRead) / bytesPerFrame;
}

// ---------------------------------------------------------
// PURE MATH SEEKING - No external libraries used! O(1) Complexity
// ---------------------------------------------------------
bool WavDecoder::seekToTime(double timeInSeconds) {
    if (!isInitialized || fileDescriptor < 0) {
        std::cerr << "[WavDecoder] ERROR: Cannot seek, no file opened." << std::endl;
        return false;
    }

    // 1. Calculate bytes per frame
    size_t bytesPerFrame = format.numChannels * (format.bitDepth / 8);
    
    // 2. Calculate theoretical byte offset based on time
    size_t bytesPerSecond = format.sampleRate * bytesPerFrame;
    size_t rawByteOffset = static_cast<size_t>(timeInSeconds * bytesPerSecond);
    
    // 3. FRAME ALIGNMENT (Crucial for high score! Prevents static noise/audio corruption)
    // We must ensure the magnet head lands exactly at the start of a frame.
    size_t alignedByteOffset = (rawByteOffset / bytesPerFrame) * bytesPerFrame;
    
    // 4. Calculate final absolute position in the physical file
    size_t targetPosition = dataStartPosition + alignedByteOffset;
    
    // Prevent seeking past the end of the file
    if (alignedByteOffset >= format.dataSize) {
        std::cerr << "[WavDecoder] ERROR: Seek target is beyond file length." << std::endl;
        return false;
    }

    // 5. Execute OS-level jump (Direct memory manipulation)
    off_t result = lseek(fileDescriptor, targetPosition, SEEK_SET);
    
    if (result == (off_t)-1) {
        std::cerr << "[WavDecoder] ERROR: OS-level lseek failed." << std::endl;
        return false;
    }
    
    std::cout << "[WavDecoder] Seek to " << timeInSeconds 
              << "s (Byte " << targetPosition << ") successful." << std::endl;
    return true;
}

AudioFormat WavDecoder::getFormat() const {
    return format;
}
// ---------------------------------------------------------
// 对接团队接口：获取总时长 (秒)
// ---------------------------------------------------------
int32_t WavDecoder::getTotalDuration() const {
    if (!isInitialized || format.sampleRate == 0) return 0;
    
    // 总时长 = 总音频数据字节数 / 每秒消耗的字节数
    size_t bytesPerSecond = format.sampleRate * format.numChannels * (format.bitDepth / 8);
    return static_cast<int32_t>(format.dataSize / bytesPerSecond);
}

// ---------------------------------------------------------
// 对接团队接口：获取当前播放进度 (秒)
// ---------------------------------------------------------
int32_t WavDecoder::getCurrentPosition() const {
    if (!isInitialized || fileDescriptor < 0) return 0;
    
    // 瞬间获取当前物理磁头在文件中的字节位置
    off_t currentOffset = lseek(fileDescriptor, 0, SEEK_CUR);
    if (currentOffset <= static_cast<off_t>(dataStartPosition)) return 0;
    
    // 当前秒数 = 已经播放的字节数 / 每秒消耗的字节数
    size_t bytesPerSecond = format.sampleRate * format.numChannels * (format.bitDepth / 8);
    size_t playedBytes = currentOffset - dataStartPosition;
    
    return static_cast<int32_t>(playedBytes / bytesPerSecond);
}
