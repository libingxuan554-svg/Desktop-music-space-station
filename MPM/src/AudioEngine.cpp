#include "../include/AudioEngine.hpp"
#include "../include/HardwareController.hpp"
#include <alsa/asoundlib.h>
#include <iostream>
#include <cmath>
#include <algorithm>
#include <vector>

// ==========================================
// 1. Core algorithm: Goertzel spectrum extraction
// ==========================================
float goertzelMagnitude(int numSamples, int targetFreq, int sampleRate, const float* data) {
    float k = 0.5f + ((float)numSamples * targetFreq) / sampleRate;
    float omega = (2.0f * M_PI * k) / numSamples;
    float sine = std::sin(omega);
    float cosine = std::cos(omega);
    float coeff = 2.0f * cosine;
    float q0 = 0, q1 = 0, q2 = 0;

    for (int i = 0; i < numSamples; ++i) {
        q0 = coeff * q1 - q2 + data[i];
        q2 = q1;
        q1 = q0;
    }
    return std::sqrt(q1 * q1 + q2 * q2 - q1 * q2 * coeff);
}

// ==========================================
//2. Engine construction and lifecycle management
// ==========================================
AudioEngine::AudioEngine(AudioSource* src, HardwareController* hw) 
    : source(src), hwController(hw), pcmHandle(nullptr), running(false) {}

AudioEngine::~AudioEngine() {
    stop();
}

bool AudioEngine::init(const char* device, unsigned int sampleRate) {
    int err;
    // Open the ALSA PCM playback device
    if ((err = snd_pcm_open(&pcmHandle, device, SND_PCM_STREAM_PLAYBACK, 0)) < 0) {
        std::cerr << "ALSA audio device open failed: " << snd_strerror(err) << std::endl;
        return false;
    }

  // Set hardware parameters
    // (16-bit little endian, interleaved mode, stereo, sample rate)
    if ((err = snd_pcm_set_params(pcmHandle, SND_PCM_FORMAT_S16_LE, SND_PCM_ACCESS_RW_INTERLEAVED, 2, sampleRate, 1, 500000)) < 0) {
        std::cerr << "ALSA parameter setup failed: " << snd_strerror(err) << std::endl;
        return false;
    }
    return true;
}

void AudioEngine::start() {
    if (running) return;
    running = true;
    playbackThread = std::thread(&AudioEngine::playbackWorker, this);
}

void AudioEngine::stop() {
    running = false;
    if (playbackThread.joinable()) {
        playbackThread.join();
    }
    if (pcmHandle) {
        snd_pcm_close(pcmHandle);
        pcmHandle = nullptr;
    }
}

void AudioEngine::recoverFromError(int err) {
    if (err == -EPIPE) {
        snd_pcm_prepare(pcmHandle); // Recover from audio underrun (XRUN)
    }
}

// ==========================================
// 3. Core audio pipeline(fetch data -> analyze spectrum -> send lighting data -> play audio)
// ==========================================
void AudioEngine::playbackWorker() {
    const int periodSize = 1024;
    const int sampleRate = 44100;
    
    std::vector<float> floatBuffer(periodSize * 2, 0.0f); // Stereo floating-point buffer
    std::vector<short> shortBuffer(periodSize * 2, 0);    // 16-bit buffer required by ALSA playback
    std::vector<float> mono(periodSize, 0.0f);            // Mono mix buffer for spectrum analysis

    // Seven representative frequencies (Hz)corresponding to violet, blue, cyan, green, yellow, orange, and red
    std::vector<int> targetFreqs = {60, 150, 400, 1000, 2000, 4000, 8000};
    std::vector<float> spectrum(7, 0.0f);

    while (running) {
        if (source) {
            // Step A: Request one frame of clean audio data from the decoder
            source->onProcessAudio(floatBuffer, periodSize);

            // Step B: Mix left and right channels into mono for lighting spectrum analysis
            for (int i = 0; i < periodSize; ++i) {
                mono[i] = (floatBuffer[i * 2] + floatBuffer[i * 2 + 1]) * 0.5f;
            }

            // Step C: Use the Goertzel algorithm to extract the energy of these seven target frequencies
            float maxMag = 0.0f;
            for (size_t i = 0; i < targetFreqs.size(); ++i) {
                spectrum[i] = goertzelMagnitude(periodSize, targetFreqs[i], sampleRate, mono.data());
                if (spectrum[i] > maxMag) maxMag = spectrum[i];
            }

            // Step D: Normalize the data(prevent clipping and over-bright lighting output)
            if (maxMag > 1e-6f) {
                for (float& v : spectrum) v /= maxMag;
            } else {
                std::fill(spectrum.begin(), spectrum.end(), 0.0f);
            }

            // Step E: Send the calculated lighting data to the hardware controller immediately (non-blocking)
            if (hwController) {
                hwController->updateLighting(spectrum);
            }

            // Step F: Convert floating-point audio back to 16-bit PCM for playback through the output device
            for (int i = 0; i < periodSize * 2; ++i) {
                float val = floatBuffer[i];
                if (val > 1.0f) val = 1.0f;
                if (val < -1.0f) val = -1.0f;
                shortBuffer[i] = static_cast<short>(val * 32767.0f);
            }

            // Step G: Perform the actual playback(write audio data into the ALSA layer)
            int err = snd_pcm_writei(pcmHandle, shortBuffer.data(), periodSize);
            if (err < 0) {
                recoverFromError(err);
            }
        } else {
            // If no music is ready yet, sleep briefly to avoid CPU busy waiting
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
        }
    }
}
