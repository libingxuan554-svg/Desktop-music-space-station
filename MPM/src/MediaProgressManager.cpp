#include "../include/MediaProgressManager.hpp"
#include <iostream>

/**
 * @brief Initializes the manager via Dependency Injection.
 * @note [Real-Time Constraints]:
 * - Zero Allocation: Binds the existing decoder instance directly, strictly avoiding dynamic heap allocation (`new`/`delete`) during instantiation.
 */
MediaProgressManager::MediaProgressManager(WavDecoder* dec) : decoder(dec) {}

/**
 * @brief Dispatches UI progress commands to the underlying decoder.
 * @note [Real-Time Constraints]:
 * - O(1) Delegation: Instantly forwards seek operations without executing the heavy disk I/O itself, ensuring the UI thread never blocks or freezes.
 */
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

/**
 * @brief Injects deterministic playback time data into the UI payload.
 * @note [Real-Time Constraints]:
 * - Wait-Free Read: Polls the current position in strict O(1) time. Guarantees that the 30FPS UI renderer never stalls waiting for lower-level mutexes.
 */
void MediaProgressManager::injectTimeData(System::PlaybackStatus& status) {
    if (!decoder) return;

    // 1. Get the current playback position in seconds
    status.currentPosition = static_cast<float>(decoder->getCurrentPosition());

    //  2. Get the total duration in seconds
    status.totalDuration = static_cast<float>(decoder->getTotalDuration());

}
