#ifndef AUDIO_SOURCE_HPP
#define AUDIO_SOURCE_HPP

#include <vector>
#include <cstdint>

/**
 * @class AudioSource
 * @brief Abstract interface for audio data providers.
 * @note [Architecture / SOLID]:
 * - Single Responsibility Principle (SRP): Defines a pure contract for supplying audio frames, isolating data provision from hardware playback logic.
 * - Dependency Inversion Principle (DIP): Serves as the abstraction layer allowing the AudioEngine to consume data without depending on concrete file decoders (e.g., WAV).
 * - Open/Closed Principle (OCP): New audio formats can be supported by creating new derived classes without modifying the existing AudioEngine.
 */
class AudioSource {
public:
    virtual ~AudioSource() = default;

    /**
 * @brief Callback triggered when the engine requires new audio data.
 * @param[out] buffer Reference to the buffer to be filled with interleaved float samples.
 * @param[in] numFrames Number of audio frames (e.g., stereo pairs) requested by the hardware.
 * @note [Real-Time Constraints]:
 * - Deterministic Execution: This method is called directly by the real-time audio thread. Concrete implementations MUST fetch data from a Lock-Free RingBuffer in O(1) time and absolutely avoid disk I/O, dynamic memory allocation (`new`/`delete`), or blocking mutexes to prevent audio stutter.
 */
    virtual void onProcessAudio(std::vector<float>& buffer, uint32_t numFrames) = 0;
};

#endif
