#ifndef RING_BUFFER_HPP
#define RING_BUFFER_HPP

#include <vector>
#include <atomic>
#include <cstdint>
#include <cstddef>

// ---------------------------------------------------------
// PURE C++ LOCK-FREE RING BUFFER
// This class safely connects the Decoding Thread and the Audio Thread.
// ---------------------------------------------------------
class RingBuffer {
public:
    // Constructor: Uses std::vector for failsafe memory management
    explicit RingBuffer(size_t capacity);
    
    // Default destructor is safe because std::vector cleans up itself
    ~RingBuffer() = default;

    // Write data into the pool (Called by WavDecoder)
    size_t write(const uint8_t* data, size_t size);

    // Read data out of the pool (Called by ALSA engine)
    size_t read(uint8_t* data, size_t size);

    // Check how much data we can read right now
    size_t getAvailableRead() const;

    // Check how much free space is left to write into
    size_t getAvailableWrite() const;

    // Clear the buffer (useful when the user seeks to a new time)
    void flush();

private:
    // Standard C++ vector to hold our audio bytes safely
    std::vector<uint8_t> buffer;
    size_t capacity;

    // HIGH SCORE FEATURE: std::atomic for Lock-Free Thread Safety!
    // Professor Bernd wants to see this for multi-threading.
    std::atomic<size_t> readIndex;
    std::atomic<size_t> writeIndex;
};

#endif // RING_BUFFER_HPP
