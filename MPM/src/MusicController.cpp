#include "../include/MusicController.hpp"
#include <iostream>

MusicController::MusicController(WavDecoder* decoder, RingBuffer* buffer)
    : m_decoder(decoder), m_ringBuffer(buffer), m_isPlaying(true), m_currentTrackIndex(0) {}

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
            break;
        case System::CommandType::NEXT_TRACK:
            if (!m_playlist.empty()) {
                m_currentTrackIndex = (m_currentTrackIndex + 1) % m_playlist.size();
                m_ringBuffer->flush(); 
                m_decoder->open(m_playlist[m_currentTrackIndex]);
            }
            break;
        case System::CommandType::PREV_TRACK:
            if (!m_playlist.empty()) {
                m_currentTrackIndex = (m_currentTrackIndex == 0) ? (m_playlist.size() - 1) : (m_currentTrackIndex - 1);
                m_ringBuffer->flush();
                m_decoder->open(m_playlist[m_currentTrackIndex]);
            }
            break;
        case System::CommandType::SEEK_FORWARD:
            if (m_decoder) {
                m_ringBuffer->flush();
                m_decoder->seekTo(cmd.floatValue);
            }
            break;
        default: break;
    }
}

void MusicController::onProcessAudio(std::vector<float>& buffer, uint32_t numFrames) {
    if (m_isPlaying && m_decoder) m_decoder->onProcessAudio(buffer, numFrames);
    else std::fill(buffer.begin(), buffer.end(), 0.0f);
}
