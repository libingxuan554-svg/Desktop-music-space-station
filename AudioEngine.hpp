#ifndef AUDIO_ENGINE_HPP
#define AUDIO_ENGINE_HPP

#include <alsa/asoundlib.h>
#include <thread>
#include <atomic>
#include <vector>
#include "AudioSource.hpp"

/**
 * @brief Audio Engine class encapsulating the ALSA driver.
 * Handles hardware initialization, real-time thread management, and error recovery.
 */
class AudioEngine {
public:
    AudioEngine(AudioSource* source);
    ~AudioEngine();

    /**
     * @brief Configures ALSA hardware parameters.
     * @param device Device name (default is "default").
     * @param rate Desired sample rate (e.g., 44100).
     * @return true if successfully initialized.
     */
    bool init(const char* device = "default", unsigned int rate = 44100);

    /**
     * @brief Starts the high-priority playback worker thread.
     */
    void start();

    /**
     * @brief Stops the playback worker thread and closes the device.
     */
    void stop();

private:
    /**
     * @brief Core worker loop synchronized by blocking I/O.
     */
    void playbackWorker();
    
    /**
     * @brief Handles PCM errors such as underruns (XRUNs).
     * @param err Negative error code from ALSA functions.
     */
    void recoverFromError(int err);

    snd_pcm_t* pcmHandle = nullptr;     // Handle for the PCM device
    AudioSource* audioSource = nullptr; // Callback target for audio processing
    
    std::thread workerThread;           // Background playback thread
    std::atomic<bool> running{false};   // Atomic flag to control thread execution

    unsigned int sampleRate = 44100;    
    const snd_pcm_uframes_t periodSize = 512; // Standard period size for low-latency RT
};

#endif
