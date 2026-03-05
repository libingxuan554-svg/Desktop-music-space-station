#ifndef MEDIA_PROGRESS_MANAGER_HPP
#define MEDIA_PROGRESS_MANAGER_HPP

#include "WavDecoder.hpp"
#include "RingBuffer.hpp"

// The "Control Center" connecting the UI Progress Bar to the Audio Engine
class MediaProgressManager {
public:
    // Pass the pointers of your decoder and buffer to this manager
    MediaProgressManager(WavDecoder* dec, RingBuffer* buf);
    ~MediaProgressManager() = default;

    // Called by the UI thread when the user drags the progress bar
    bool handleUserSeek(double targetTimeSeconds);

private:
    WavDecoder* decoder;
    RingBuffer* ringBuffer;
};

#endif // MEDIA_PROGRESS_MANAGER_HPP
