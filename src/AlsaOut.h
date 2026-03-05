#pragma once
#include <alsa/asoundlib.h>

class AlsaOut {
public:
    bool open(unsigned int sampleRate, int channels);
    void close();

    // interleaved int16 frames
    void writeFrames(const int16_t* data, snd_pcm_sframes_t frames);

    // returns true if pause is supported and succeeded
    bool pause(bool on);

    unsigned int rate() const { return actualRate_; }
    int channels() const { return channels_; }
    snd_pcm_uframes_t periodFrames() const { return periodFrames_; }

private:
    snd_pcm_t* pcm_ = nullptr;
    unsigned int actualRate_ = 0;
    int channels_ = 0;
    snd_pcm_uframes_t periodFrames_ = 512;
    snd_pcm_uframes_t bufferFrames_ = 2048;
};
