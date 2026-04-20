#ifndef MUSIC_CONTROLLER_HPP
#define MUSIC_CONTROLLER_HPP

#include <vector>
#include <string>
#include <atomic>
#include "../../SystemInterfaces.hpp"
#include "WavDecoder.hpp"
#include "RingBuffer.hpp"
#include "AudioSource.hpp"

/**
 * @class MusicController
 * @brief Core orchestrator bridging decoding, buffering, and playback state.
 * @note [Architecture / SOLID]:
 * - Single Responsibility Principle (SRP): Manages playback state and playlists exclusively; delegates decoding to WavDecoder and buffering to RingBuffer.
 * - Dependency Inversion Principle (DIP): Interacts via injected pointers rather than instantiating dependencies internally.
 */
class MusicController : public AudioSource {
public:
    /**
 * @brief Initializes the controller via Dependency Injection.
 * @param[in] decoder Pointer to the WAV decoding module.
 * @param[in] buffer Pointer to the lock-free ring buffer.
 * @note [Real-Time Constraints]:
 * - Dependency Injection: Avoids runtime dynamic memory allocation (`new`/`delete`) to guarantee initialization determinism.
 */
    MusicController(WavDecoder* decoder, RingBuffer* buffer);

    void setPlaylist(const std::vector<std::string>& playlist);
    /**
 * @brief Processes UI-triggered playback commands (Play/Pause/Next).
 * @param[in] cmd The control command struct from the UI layer.
 * @note [Real-Time Constraints]:
 * - Execution Determinism: O(1) state mutation. Strictly prohibits blocking file I/O during track switching to prevent UI thread freezing.
 */
    void processCommand(const System::ControlCommand& cmd); // Ensure the return type is void
    /**
 * @brief Fulfills the AudioSource contract to supply PCM data to the hardware.
 * @param[out] buffer Buffer to be populated with interleaved audio samples.
 * @param[in] numFrames Requested number of audio frames.
 * @note [Real-Time Constraints]:
 * - Lock-Free Fetching: Executes strictly in O(1) time by pulling from the `RingBuffer`. No blocking locks are acquired in the real-time ALSA thread.
 */
    void onProcessAudio(std::vector<float>& buffer, uint32_t numFrames) override;
/**
 * @brief Safely retrieves the current playback status for UI polling.
 * @return True if currently playing.
 * @note [Real-Time Constraints]:
 * - Thread Safety: Utilizes `std::atomic<bool>` for lock-free read access, preventing data races between the playback engine and UI renderer.
 */
    bool isPlaying() const { return m_isPlaying; }
//  Newly added: get the index of the current track
    int getCurrentTrackIndex() const {
        return m_currentTrackIndex;
    }
//Newly added: get the current volume value (0.0 - 1.0)
    float getVolume() const {
        return m_volume;
    }

private:
    WavDecoder* m_decoder;            // Missing field related to the reported error
    RingBuffer* m_ringBuffer;         // Missing field related to the reported error
    std::vector<std::string> m_playlist; // Missing field related to the reported error
    std::atomic<bool> m_isPlaying;    // Missing field related to the reported error
    int m_currentTrackIndex;          // Missing field related to the reported error
    float m_volume;
};

#endif
