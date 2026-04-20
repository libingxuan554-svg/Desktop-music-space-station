#include "../include/MusicController.hpp"
#include <iostream>
#include <algorithm>

// Constructor: initialize m_volume to 0.5f (default 50% volume)
/**
 * @brief Initializes playback state and binds injected dependencies.
 * @note [Real-Time Constraints]:
 * - Zero Allocation: Performs strictly lightweight variable initialization (O(1)), completely avoiding runtime heap memory overhead.
 */
MusicController::MusicController(WavDecoder* decoder, RingBuffer* buffer)
    : m_decoder(decoder), m_ringBuffer(buffer), m_isPlaying(true),
      m_currentTrackIndex(0), m_volume(0.5f) {}

/**
 * @brief Dispatches and mutates playback states based on UI events.
 * @note [Real-Time Constraints]:
 * - State Mutation Safety: Employs a pure O(1) `flush()` on the lock-free ring buffer during track switching or seeking, absolutely eliminating audio tearing and residual sound without stalling the UI thread.
 */
void MusicController::setPlaylist(const std::vector<std::string>& playlist) {
    m_playlist = playlist;
    if (!m_playlist.empty()) {
        m_currentTrackIndex = 0;
        m_decoder->open(m_playlist[m_currentTrackIndex]);
    }
}

void MusicController::processCommand(const System::ControlCommand& cmd) {
    switch (cmd.type) {
        case System::CommandType::PLAY_PAUSE:
            m_isPlaying = !m_isPlaying;
            std::cout << (m_isPlaying ? "▶️ Playing" : "⏸️ Paused") << std::endl;
            break;

// Restore the logic for selecting and playing a specific song
        case System::CommandType::SELECT_SONG:
            // Make sure the incoming song index (cmd.intValue) is within range
            if (!m_playlist.empty() && cmd.intValue >= 0 && cmd.intValue < static_cast<int>(m_playlist.size())) {
                m_currentTrackIndex = cmd.intValue;
                m_ringBuffer->flush(); // Clear the buffer before switching tracks to avoid leftover sound
                m_decoder->open(m_playlist[m_currentTrackIndex]); // Open the new track
                m_isPlaying = true;    // Start playback immediately
                std::cout << "🔀 Selected track: " << m_playlist[m_currentTrackIndex] << std::endl;
            }
            break;

        case System::CommandType::NEXT_TRACK:
            if (!m_playlist.empty()) {
                m_currentTrackIndex = (m_currentTrackIndex + 1) % m_playlist.size();
                m_ringBuffer->flush(); // Clear buffer to avoid audio overlap when switching tracks
                m_decoder->open(m_playlist[m_currentTrackIndex]);
                m_isPlaying = true;
                std::cout << "⏭️ Next track: " << m_playlist[m_currentTrackIndex] << std::endl;
            }
            break;

        case System::CommandType::PREV_TRACK:
            if (!m_playlist.empty()) {
                m_currentTrackIndex = (m_currentTrackIndex == 0) ? 
                                      (m_playlist.size() - 1) : (m_currentTrackIndex - 1);
                m_ringBuffer->flush(); // Clear buffer to avoid audio overlap when switching tracks
                m_decoder->open(m_playlist[m_currentTrackIndex]);
                m_isPlaying = true;
                std::cout << "⏮️ Previous track: " << m_playlist[m_currentTrackIndex] << std::endl;
            }
            break;

        case System::CommandType::SEEK_FORWARD:
            if (m_decoder && !m_playlist.empty()) {
                m_ringBuffer->flush(); // Clear buffer before seeking to avoid leftover sound
                m_decoder->seekTo(cmd.floatValue);
            }
            break;

        // 🔊 Actual volume control
        case System::CommandType::VOLUME_UP:
            m_volume = std::min(1.0f, m_volume + 0.05f);
            std::cout << "🔊 Volume: " << static_cast<int>(m_volume * 100) << "%" << std::endl;
            break;

        case System::CommandType::VOLUME_DOWN:
            m_volume = std::max(0.0f, m_volume - 0.05f);
            std::cout << "🔉 Volume: " << static_cast<int>(m_volume * 100) << "%" << std::endl;
            break;

        default:
            break;
    }
}

/**
 * @brief The hardware-triggered callback supplying final PCM data.
 * @note [Real-Time Constraints]:
 * - O(N) DSP Execution: Applies volume scaling using strict linear memory traversal. Pure CPU mathematical operation guarantees execution within the strict ALSA deadline without kernel context switching.
 */
void MusicController::onProcessAudio(std::vector<float>& buffer, uint32_t numFrames) {
    if (m_isPlaying && m_decoder) {
        // 1. Get the original audio data
        m_decoder->onProcessAudio(buffer, numFrames);

        // 2. Apply the volume multiplier to every audio sample
        for (size_t i = 0; i < buffer.size(); ++i) {
            buffer[i] *= m_volume;
        }
    } else {
        // Output pure silence when paused
        std::fill(buffer.begin(), buffer.end(), 0.0f);
    }
}
