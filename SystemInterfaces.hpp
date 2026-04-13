#ifndef SYSTEM_INTERFACES_HPP
#define SYSTEM_INTERFACES_HPP

#include <string>    //  Include standard string class
#include <vector>    //  Include dynamic array container
#include <cstdint>   //  Include standard integer definitions

namespace System { //  Define global system-level communication namespace

    /**
     * @enum CommandType
     * @brief Command interface (UI -> Audio/System logic)
     * Used to send control commands. The UI module is the Publisher.
     * @note [Architecture / SOLID]:
     * Dependency Inversion Principle (DIP): Defines abstract control contracts, completely decoupling UI components from low-level hardware control.
     */
    enum class CommandType {
        PLAY_PAUSE,    // Play or pause toggle
        NEXT_TRACK,    // Switch to next track
        PREV_TRACK,    // Switch to previous track
        VOLUME_UP,     // Increase volume
        VOLUME_DOWN,   // Decrease volume
        SELECT_SONG,   // Used with intValue (song index)
        SEEK_FORWARD,  // Fast forward progress
        SEEK_BACKWARD, // Rewind progress
        ENTER_STANDBY  // Enter standby mode
    };

    /**
     * @struct ControlCommand
     * @brief Entity data packet carrying control commands
     */
    struct ControlCommand {
        CommandType type;        // Command type enumeration
        int32_t intValue = 0;    // Auxiliary parameter, such as volume value or song ID
        float floatValue = 0.0f; // Floating-point auxiliary parameter, used for percentage progress, etc.
    };

    /**
     * @struct PlaybackStatus
     * @brief Playback status interface (Audio -> UI)
     * The UI module subscribes to this data to update song names, progress bars, etc.
     * @note [Architecture / SOLID]:
     * Single Responsibility Principle (SRP): Acts as a pure data container (POD) responsible only for passing state synchronization, containing no business processing logic.
     */
    struct PlaybackStatus {
        std::string songName;    // Name of the currently playing song
        std::string artist;      // Artist name information
        int32_t currentPosition; // Current playback position in seconds
        int32_t totalDuration;   // Total duration in seconds
        bool isPlaying;          // Whether it is currently in an active playback state
        int32_t volume;          // Current real hardware volume (0-100)
        
        // Store all scanned local song names
        std::vector<std::string> playlist; // Global music playlist collection
    };

    /**
     * @struct AudioVisualData
     * @brief Real-time audio stream interface (Audio -> UI/LedStrip)
     * Used for spectrum animation and LED strip rhythm. High-frequency updates.
     * @note [Real-Time Constraints]:
     * // [REAL-TIME COMPLIANCE]:
     * // This structure is written out at high frequency by the audio interrupt thread; its extremely flat design ensures extreme O(1) efficiency for the main render thread to fetch data.
     */
    struct AudioVisualData {
        float overallIntensity;         // Overall loudness (0.0-1.0), interfaces with your renderMirrorEqualizer
        std::vector<float> frequencies; // Frequency band data (e.g., 16 bands), used for advanced spectrograms
    };

    /**
     * @struct EnvironmentStatus
     * @brief Environment data interface (Sensor -> UI/System logic)
     * Used for dashboard display on the standby page.
     */
    struct EnvironmentStatus {
        float temperature;     // Reserved for your env module (Physical room temperature)
        float humidity;        // Reserved for your env module (Physical humidity)
        int weatherCode;       // Reserved for your network weather module (e.g., 0=Sunny, 1=Cloudy, 2=Rainy)
        int cpuLoadPercent;    // Real-time CPU load percentage
        int memUsagePercent;   // Real-time memory usage percentage
        uint32_t lux;          // Addition: Light intensity (can be used to auto-adjust UI brightness)
    };

} // namespace System

#endif
