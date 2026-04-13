#ifndef WAV_DECODER_HPP
#define WAV_DECODER_HPP

#include <string>  
#include <vector>  
#include <fstream> 
#include <cstdint>

// ==========================================
/**
 * @class WavDecoder
 * @brief Responsible for parsing WAV format audio files and providing a continuous PCM data stream for the low-level audio engine.
 * @note [Architecture / SOLID]:
 * - Follows SRP: Focuses solely on unpacking and reading WAV file formats, not involving playback device control or hardware handshaking.
 * - RAII: Safely manages the lifecycle of std::ifstream internally, ensuring no file handle leaks.
 */
class WavDecoder {
public:
// ==========================================
/**
 * @brief Default constructor: Initialize decoder idle state
 */
    WavDecoder(); // Create and reset the decoder component
// ==========================================

// ==========================================
/**
 * @brief  Destructor: Safely release file stream
 */
    ~WavDecoder(); //  Automatically clean up low-level resources to avoid zombie handles
// ==========================================

// ==========================================
/**
 * @brief  Open and parse the header information of the specified WAV file
 * @param[in] path  Physical path of the audio file
 * @return  Whether the file is valid and successfully parsed
 * @note [Real-Time Constraints]:
 * - This operation contains high-cost disk I/O, must be completed in advance during the track-changing state machine phase, and is strictly forbidden to be called in the audio callback thread.
 */
    bool open(const std::string& path); //  Mount disk file and extract audio metadata
// ==========================================

// ==========================================
/**
 * @brief Audio callback pull interface: Fill PCM samples into low-level hardware buffer
 * @param[out] buffer  Target buffer provided by the audio driver layer
 * @param[in] numFrames  Number of audio frames to read
 * @note [Real-Time Constraints]:
 * // [REAL-TIME COMPLIANCE]:
 * //  This method runs directly in the highest-priority audio interrupt thread (ALSA Callback).
 * //  Internally uses on-demand batch reading, must fill the DMA buffer within an extremely short time budget, and strictly forbids any lock operations that could suspend the thread.
 */
    void onProcessAudio(std::vector<float>& buffer, uint32_t numFrames); // Real-time delivery of decompressed PCM floating-point audio stream
// ==========================================

// ==========================================
/**
 * @brief Extreme-speed timeline seeking based on percentage ratio
 * @param[in] progress  Percentage of target progress (0.0f - 1.0f)
 * @note [Real-Time Constraints]:
 * // [REAL-TIME COMPLIANCE]:
 * // Pure O(1) complexity mathematical multiplication and fseek file pointer offset, no time-consuming search traversals.
 * // Ensures that high-frequency seeks caused by the user dragging the progress bar on the touchscreen do not cause CPU peak lagging.
 */
    void seekTo(float progress); // Instantly reset data stream pointer based on UI ratio
// ==========================================
    
// ==========================================
/**
 * @brief Timeline seeking based on absolute seconds
 * @param[in] seconds Target time (seconds)
 * @return Whether the seek command successfully reached a valid data area
 */
    bool seekToTime(double seconds); // Locate to a specific precise playback timestamp
// ==========================================

// ==========================================
/**
 * @brief  Get the exact currently played time
 * @return  Current playback progress (seconds)
 */
    double getCurrentPosition(); //  Calculate cumulative absolute duration flowing through the audio engine
// ==========================================

// ==========================================
/**
 * @brief  Get the total duration of the current audio file
 * @return  Total audio duration (seconds)
 */
    double getTotalDuration() const; //  Deduce overall playback length based on sample rate, bit depth, and total data volume
// ==========================================

private:
    std::ifstream file; // Long-held low-level binary file stream read handle
    uint32_t totalDataSize; //  Parsed total byte size of pure PCM data block
    int sampleRate; // Inherent hardware sample rate of the audio file (e.g., 44100Hz)
    int channels; // Number of hardware audio channels in the file (e.g., 2 for stereo)
};

#endif
