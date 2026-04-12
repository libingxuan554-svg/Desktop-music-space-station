#include "../include/MediaProgressManager.hpp"
#include <iostream>

// Constructor: bind the decoder instance
MediaProgressManager::MediaProgressManager(WavDecoder* dec) : decoder(dec) {}

// Handle progress control commands from the UI
bool MediaProgressManager::processCommand(const System::ControlCommand& cmd) {
    if (!decoder) return false;

    // 1. If the command is a percentage-based seek (0.0 ~ 1.0)
    if (cmd.type == System::CommandType::SEEK_FORWARD) {
        decoder->seekTo(cmd.floatValue);
        return true;
    }

    //2. If the command is a seek to a specific time in seconds
    if (cmd.type == System::CommandType::SEEK_BACKWARD) {
        return decoder->seekToTime(static_cast<double>(cmd.floatValue));
    }

    return false;
}

// Core function: inject time data for UI progress display
void MediaProgressManager::injectTimeData(System::PlaybackStatus& status) {
    if (!decoder) return;

    // 1. Get the current playback position in seconds
    status.currentPosition = static_cast<float>(decoder->getCurrentPosition());

    //  2. Get the total duration in seconds
    status.totalDuration = static_cast<float>(decoder->getTotalDuration());

}
