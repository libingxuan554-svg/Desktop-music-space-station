bool WavDecoder::openFile(const std::string& filepath) {
    if (isInitialized) {
        closeFile();
    }

    fileDescriptor = open(filepath.c_str(), O_RDONLY);
    if (fileDescriptor < 0) {
        std::cerr << "[WavDecoder] ERROR: Could not open file: " << filepath << std::endl;
        return false;
    }

    // 1. 读取基础头部 (只读前 36 字节，到 'data' 之前)
    uint8_t header[36];
    if (read(fileDescriptor, header, 36) != 36) {
        std::cerr << "[WavDecoder] ERROR: File is too small." << std::endl;
        closeFile();
        return false;
    }

    // 2. 解析基础格式信息
    format.numChannels = header[22] | (header[23] << 8);
    format.sampleRate  = header[24] | (header[25] << 8) | (header[26] << 16) | (header[27] << 24);
    format.bitDepth    = header[34] | (header[35] << 8);

    // 3. 智能雷达：扫描寻找 "data" 块 (跳过所有垃圾信息)
    char chunkId[5] = {0}; // 用来装 4 个字母的标记
    uint32_t chunkSize = 0;
    bool foundData = false;

    while (true) {
        // 读取 4 个字母的名字 (比如 "fmt ", "LIST", "data")
        if (read(fileDescriptor, chunkId, 4) != 4) break; 
        
        // 读取这个块的大小 (4 字节)
        uint8_t sizeBuffer[4];
        if (read(fileDescriptor, sizeBuffer, 4) != 4) break;
        chunkSize = sizeBuffer[0] | (sizeBuffer[1] << 8) | (sizeBuffer[2] << 16) | (sizeBuffer[3] << 24);

        // 如果找到了 "data" 块！
        if (std::string(chunkId, 4) == "data") {
            format.dataSize = chunkSize; // 拿到真正的音频大小！
            dataStartPosition = lseek(fileDescriptor, 0, SEEK_CUR); // 记录磁头当前的精确物理位置！
            foundData = true;
            break; // 停止扫描
        } else {
            // 如果是垃圾块 (比如 LIST)，直接命令 OS 让磁头跳过这个块的大小
            lseek(fileDescriptor, chunkSize, SEEK_CUR);
        }
    }

    if (!foundData) {
        std::cerr << "[WavDecoder] ERROR: 找不到 data 块！这不是一个标准的 WAV 文件。" << std::endl;
        closeFile();
        return false;
    }

    isInitialized = true;
    std::cout << "[WavDecoder] Success! SR: " << format.sampleRate 
              << "Hz, CH: " << format.numChannels 
              << ", Depth: " << format.bitDepth << "bit." 
              << " | DataSize: " << format.dataSize << " bytes" << std::endl;
    return true;
}
