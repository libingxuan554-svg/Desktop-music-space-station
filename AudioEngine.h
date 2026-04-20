#ifndef AUDIO_ENGINE_HPP
#define AUDIO_ENGINE_HPP

#include <alsa/asoundlib.h>
#include <thread>
#include <atomic>
#include <vector>
#include "AudioSource.hpp"

// Forward declaration: there is a class named HardwareController defined elsewhere
class HardwareController; 

/**
 * @class AudioEngine
 * @brief Core ALSA audio playback engine managing the hardware audio buffer.
 * @note [Architecture / SOLID]:
 * - Single Responsibility Principle (SRP): Exclusively manages the ALSA hardware playback lifecycle. It delegates file parsing and decoding to external modules.
 * - Dependency Inversion Principle (DIP): Depends entirely on the abstract `AudioSource` interface to fetch PCM data, completely decoupling it from specific file formats (e.g., WAV).
 */
class AudioEngine {
public:
    //Core fix: this constructor now accepts two parameters
    //(the second parameter defaults to nullptr to avoid errors)
     /**
 * @brief Constructs the audio engine with injected dependencies.
 * @param[in] src Pointer to the abstract audio source interface (provides PCM data).
 * @param[in] hw Optional pointer to the hardware controller (e.g., for LED synchronization).
 * @note [Real-Time Constraints]:
 * - Initialization Phase: Executes outside the real-time audio loop. Employs Dependency Injection (DI) to avoid dynamic memory allocation or complex instantiations during runtime.
 */
    AudioEngine(AudioSource* src, HardwareController* hw = nullptr);
    ~AudioEngine();
    /**
 * @brief Initializes the ALSA PCM hardware handle and sets hardware parameters.
 * @param[in] device The ALSA hardware device name (e.g., "plughw:0,0").
 * @param[in] sampleRate The target playback sample rate in Hz.
 * @return True if hardware initialization succeeds, false otherwise.
 * @note [Real-Time Constraints]:
 * - Blocking Operation: Contains blocking hardware I/O setup. MUST be called during the system startup phase before the real-time playback thread is spawned.
 */
    bool init(const char* device, unsigned int sampleRate);
    void start();
    void stop();

private:
    void playbackWorker();
    void recoverFromError(int err);

    AudioSource* source;
    HardwareController* hwController; // Stores the reference to the hardware controller
    snd_pcm_t* pcmHandle;
    std::thread playbackThread;
    std::atomic<bool> running;
};

#endif
