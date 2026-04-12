#ifndef MUSIC_CONTROLLER_HPP
#define MUSIC_CONTROLLER_HPP

#include <vector>
#include <string>
#include <atomic>
#include "../../SystemInterfaces.hpp"
#include "WavDecoder.hpp"
#include "RingBuffer.hpp"
#include "AudioSource.hpp"

class MusicController : public AudioSource {
public:
    MusicController(WavDecoder* decoder, RingBuffer* buffer);

    void setPlaylist(const std::vector<std::string>& playlist);
    void processCommand(const System::ControlCommand& cmd); // Ensure the return type is void
    void onProcessAudio(std::vector<float>& buffer, uint32_t numFrames) override;
// Newly added: expose the current playback state
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
