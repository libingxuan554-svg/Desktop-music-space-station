#include "../include/MediaProgressManager.hpp"
#include <iostream>

MediaProgressManager::MediaProgressManager(WavDecoder* dec, RingBuffer* buf) 
    : decoder(dec), ringBuffer(buf) {
    std::cout << "[ProgressManager] Initialized and connected to Decoder & Buffer." << std::endl;
}

// ---------------------------------------------------------
// THIS IS THE COMBO MOVE FOR PROGRESS BAR DRAGGING
// ---------------------------------------------------------
bool MediaProgressManager::handleUserSeek(double targetTimeSeconds) {
    if (!decoder || !ringBuffer) {
        std::cerr << "[ProgressManager] ERROR: Missing decoder or buffer!" << std::endl;
        return false;
    }

    std::cout << "[ProgressManager] UI requested jump to: " << targetTimeSeconds << "s" << std::endl;

    // Step 1: Tell the decoder to instantly jump to the new mathematical position
    bool seekSuccess = decoder->seekToTime(targetTimeSeconds);
    
    if (seekSuccess) {
        // Step 2: Flush the ring buffer! 
        // If we don't do this, the user will hear the old music in the pool first.
        ringBuffer->flush();
        std::cout << "[ProgressManager] Seek successful, old buffer water flushed!" << std::endl;
        return true;
    }

    std::cerr << "[ProgressManager] Seek failed at decoder level." << std::endl;
    return false;
}
