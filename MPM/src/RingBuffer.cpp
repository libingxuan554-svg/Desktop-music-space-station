// src/RingBuffer.cpp
#include "../include/RingBuffer.hpp"
#include <cstring>  // for std::memcpy
#include <iostream>

// Constructor: Initialize the pool safely
// We allocate the vector memory once here to avoid memory fragmentation.
RingBuffer::RingBuffer(size_t cap) 
    : buffer(cap), capacity(cap), readIndex(0), writeIndex(0) {
    std::cout << "[RingBuffer] Created lock-free pool with capacity: " << capacity << " bytes." << std::endl;
}

// How much unread music data is waiting in the pool?
size_t RingBuffer::getAvailableRead() const {
    // Because we use std::atomic, this read is completely thread-safe!
    return writeIndex.load(std::memory_order_acquire) - readIndex.load(std::memory_order_relaxed);
}

// How much empty space is left for the WavDecoder to pour water into?
size_t RingBuffer::getAvailableWrite() const {
    return capacity - getAvailableRead();
}

// Flush everything (used when the user clicks 'Seek' to skip to a new time)
void RingBuffer::flush() {
    // Simply resetting the pointers makes the buffer "empty" instantly!
    // No need to delete memory - pure O(1) failsafe operation.
    readIndex.store(0, std::memory_order_relaxed);
    writeIndex.store(0, std::memory_order_release);
    std::cout << "[RingBuffer] Buffer flushed for Seeking." << std::endl;
}

// The Decoder calls this to push new music data into the pool
size_t RingBuffer::write(const uint8_t* data, size_t size) {
    size_t freeSpace = getAvailableWrite();
    if (freeSpace == 0 || size == 0) {
        return 0; // Pool is full! Stop pouring!
    }

    // Only write as much as we can fit
    size_t writeSize = (size > freeSpace) ? freeSpace : size;

    // Math magic: Find exactly where we are in the circular buffer
    size_t currentWritePos = writeIndex.load(std::memory_order_relaxed) % capacity;
    size_t spaceUntilEnd = capacity - currentWritePos;

    // Do we need to wrap around to the beginning of the pool?
    if (writeSize <= spaceUntilEnd) {
        // Normal case: it fits before hitting the wall
        std::memcpy(&buffer[currentWritePos], data, writeSize);
    } else {
        // Wrap-around case: pour some at the end, and the rest at the beginning
        std::memcpy(&buffer[currentWritePos], data, spaceUntilEnd);
        std::memcpy(&buffer[0], data + spaceUntilEnd, writeSize - spaceUntilEnd);
    }

    // Update the atomic write index so the Audio thread knows new data arrived
    writeIndex.fetch_add(writeSize, std::memory_order_release);

    return writeSize;
}

// The ALSA Engine calls this to suck data out of the pool to play
size_t RingBuffer::read(uint8_t* data, size_t size) {
    size_t availableData = getAvailableRead();
    if (availableData == 0 || size == 0) {
        return 0; // Pool is empty! We are starving!
    }

    // Only read what is actually available
    size_t readSize = (size > availableData) ? availableData : size;

    size_t currentReadPos = readIndex.load(std::memory_order_relaxed) % capacity;
    size_t spaceUntilEnd = capacity - currentReadPos;

    // Do we need to wrap around?
    if (readSize <= spaceUntilEnd) {
        // Normal case
        std::memcpy(data, &buffer[currentReadPos], readSize);
    } else {
        // Wrap-around case
        std::memcpy(data, &buffer[currentReadPos], spaceUntilEnd);
        std::memcpy(data + spaceUntilEnd, &buffer[0], readSize - spaceUntilEnd);
    }

    // Update the atomic read index
    readIndex.fetch_add(readSize, std::memory_order_release);

    return readSize;
}
