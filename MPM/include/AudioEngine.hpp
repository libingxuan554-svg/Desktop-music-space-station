#ifndef AUDIO_ENGINE_HPP
#define AUDIO_ENGINE_HPP

#include <alsa/asoundlib.h>
#include <thread>
#include <atomic>
#include <vector>
#include "AudioSource.hpp"

// Forward declaration: there is a class named HardwareController defined elsewhere
class HardwareController; 

class AudioEngine {
public:
    //Core fix: this constructor now accepts two parameters
    //(the second parameter defaults to nullptr to avoid errors)
    AudioEngine(AudioSource* src, HardwareController* hw = nullptr);
    ~AudioEngine();

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
