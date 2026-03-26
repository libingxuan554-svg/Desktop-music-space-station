#include "../include/AudioEngine.hpp"
#include <iostream>
#include <pthread.h>
#include <sched.h>

AudioEngine::AudioEngine(AudioSource* source) : audioSource(source) {}

AudioEngine::~AudioEngine() {
    stop();
    if (pcmHandle) {
        snd_pcm_close(pcmHandle);
    }
}

bool AudioEngine::init(const char* device, unsigned int rate) {
    sampleRate = rate;
    int err;

    // Technical Core: Open the PCM playback stream
    if ((err = snd_pcm_open(&pcmHandle, device, SND_PCM_STREAM_PLAYBACK, 0)) < 0) {
        std::cerr << "PCM open error: " << snd_strerror(err) << std::endl;
        return false;
    }

    // Configure hardware: 32-bit float, Stereo, Interleaved access
    if ((err = snd_pcm_set_params(pcmHandle,
                                  SND_PCM_FORMAT_FLOAT_LE,
                                  SND_PCM_ACCESS_RW_INTERLEAVED,
                                  2,             // 2 Channels
                                  sampleRate,
                                  1,             // Enable resampling
                                  500000)) < 0) { // 500ms latency buffer
        std::cerr << "Set params error: " << snd_strerror(err) << std::endl;
        return false;
    }

    return true;
}

void AudioEngine::start() {
    if (running) return;
    running = true;
    // Launch thread using C++ STL (Lecture 3, Page 6)
    workerThread = std::thread(&AudioEngine::playbackWorker, this);
}

void AudioEngine::stop() {
    running = false;
    if (workerThread.joinable()) {
        workerThread.join();
    }
}

void AudioEngine::playbackWorker() {
    /**
     * RT Priority Management (Lecture 3, Page 2)
     * Set SCHED_FIFO to ensure the audio thread is not preempted by standard tasks.
     */
    struct sched_param param;
    param.sched_priority = 80; // High real-time priority
    if (pthread_setschedparam(pthread_self(), SCHED_FIFO, &param) != 0) {
        std::cerr << "WARNING: Failed to set SCHED_FIFO. Check permissions." << std::endl;
    }

    // Buffer allocated once to maintain deterministic performance in the RT loop
    std::vector<float> buffer(periodSize * 2); 

    while (running) {
        // Event-Driven: Trigger the callback to get data from the technical core
        if (audioSource) {
            audioSource->onProcessAudio(buffer, periodSize);
        }

        /**
         * Blocking I/O Synchronization (Lecture 3, Page 8)
         * snd_pcm_writei blocks the thread until the hardware buffer has space.
         * The thread is woken by the kernel precisely when data is needed.
         */
        sframes_t frames = snd_pcm_writei(pcmHandle, buffer.data(), periodSize);

        // Error handling for hardware events
        if (frames < 0) {
            recoverFromError(static_cast<int>(frames));
        } else if (frames < static_cast<sframes_t>(periodSize)) {
            std::cerr << "Warning: Short write (" << frames << "/" << periodSize << " frames)." << std::endl;
        }
    }
}

void AudioEngine::recoverFromError(int err) {
    if (err == -EPIPE) { 
        // Buffer Underrun: The RT thread missed a deadline
        std::cerr << "XRUN (Underrun) detected! Re-preparing the stream." << std::endl;
        snd_pcm_prepare(pcmHandle);
    } else if (err < 0) {
        std::cerr << "ALSA Runtime Error: " << snd_strerror(err) << std::endl;
        snd_pcm_prepare(pcmHandle);
    }
}
