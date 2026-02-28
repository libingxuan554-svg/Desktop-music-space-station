#ifndef AUDIO_ENGINE_HPP
#define AUDIO_ENGINE_HPP

#include <alsa/asoundlib.h>
#include <thread>
#include <atomic>
#include <vector>
#include "AudioSource.hpp"

/**
 * @brief Audio Engine class encapsulating the ALSA driver.
 * Responsible for Technical Core duties: hiding complexity and managing the RT thread.
 */
class AudioEngine {
public:
    AudioEngine(AudioSource* source);
    ~AudioEngine();

    /**
     * @brief Configures the ALSA hardware parameters .
     * @param device Device string.
     * @param rate Target sample rate .
     * @return true if initialization is successful.
     */
    bool init(const char* device = "default", unsigned int rate = 44100);

    /**
     * @brief Starts the high-priority playback worker thread.
     */
    void start();

    /**
     * @brief Stops the playback worker thread and releases resources.
     */
    void stop();

private:
    /**
     * @brief The core execution loop using Blocking I/O.
     * Synchronized by the hardware buffer availability.
     */
    void playbackWorker();
    
    /**
     * @brief Recovers the PCM stream from runtime errors (e.g., Underruns).
     * @param err ALSA error code.
     */
    void recoverFromError(int err);

    snd_pcm_t* pcmHandle = nullptr;     // Low-level ALSA handle
    AudioSource* audioSource = nullptr; // Callback target
    
    std::thread workerThread;           // Background playback thread
    std::atomic<bool> running{false};   // Atomic control for thread lifecycle

    unsigned int sampleRate = 44100;    
    const snd_pcm_uframes_t periodSize = 512; // Standard chunk size for low-latency RT
};

#endif
