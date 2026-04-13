#ifndef MEDIA_PROGRESS_MANAGER_HPP
#define MEDIA_PROGRESS_MANAGER_HPP

#include "WavDecoder.hpp" // Include low-level WAV audio decoder dependency
#include "../../SystemInterfaces.hpp" //  Include system-level core communication interfaces

// ==========================================
/**
 * @class MediaProgressManager
 * @brief Responsible for managing and resolving audio playback progress, timeline seeking, and related logic.
 * @note [Architecture / SOLID]:
 * - Follows SRP: Focuses solely on progress and time control, decoupling this complex business logic from the main audio engine loop.
 * - Dependency Injection (DI): Injects WavDecoder via pointer, avoiding hard-coded dependencies.
 */
class MediaProgressManager {
public:
// ==========================================
/**
 * @brief  Constructor: Inject audio decoder instance
 * @param[in] dec  Pointer to the underlying audio decoder
 */
    MediaProgressManager(WavDecoder* dec); //  Initialize the media progress manager
// ==========================================

// ==========================================
/**
 * @brief  Process media control commands from the bus (e.g., fast forward, rewind, seek)
 * @param[in] cmd  System control command structure
 * @return  Whether the command was successfully recognized and executed
 * @note [Real-Time Constraints]:
 * // [REAL-TIME COMPLIANCE]:
 * // Execution of this method must return synchronously in a very short time to avoid blocking the distribution control queue. Internal file offset calculation should be kept at O(1) complexity.
 */
    bool processCommand(const System::ControlCommand& cmd); //  Intercept and parse progress-related control commands
// ==========================================

// ==========================================
/**
 * @brief Inject the latest playback time and progress data into the system's global state machine
 * @param[out] status  Reference to the system's playback status structure
 * @note [Real-Time Constraints]:
 * // [REAL-TIME COMPLIANCE]:
 * // Uses Pass-by-Reference to modify memory directly, completely avoiding time-consuming and memory-allocation overhead caused by structure copying, ensuring high-efficiency operation of the audio thread.
 */
    void injectTimeData(System::PlaybackStatus& status); //  Update total duration and current playback position in real-time
// ==========================================

private:
    WavDecoder* decoder; // Caches the pointer to the decoder, used for mapping data frames to time
};

#endif
