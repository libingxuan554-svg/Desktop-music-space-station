#include "AlsaOut.h"
#include <iostream>

bool AlsaOut::open(unsigned int sampleRate, int channels) {
    channels_ = channels;

    int rc = snd_pcm_open(&pcm_, "default", SND_PCM_STREAM_PLAYBACK, 0);
    if (rc < 0) {
        std::cerr << "snd_pcm_open: " << snd_strerror(rc) << "\n";
        pcm_ = nullptr;
        return false;
    }

    snd_pcm_hw_params_t* params = nullptr;
    snd_pcm_hw_params_alloca(&params);
    snd_pcm_hw_params_any(pcm_, params);

    rc = snd_pcm_hw_params_set_access(pcm_, params, SND_PCM_ACCESS_RW_INTERLEAVED);
    if (rc < 0) { std::cerr << "set_access: " << snd_strerror(rc) << "\n"; return false; }

    rc = snd_pcm_hw_params_set_format(pcm_, params, SND_PCM_FORMAT_S16_LE);
    if (rc < 0) { std::cerr << "set_format: " << snd_strerror(rc) << "\n"; return false; }

    rc = snd_pcm_hw_params_set_channels(pcm_, params, channels_);
    if (rc < 0) { std::cerr << "set_channels: " << snd_strerror(rc) << "\n"; return false; }

    unsigned int rate = sampleRate;
    int dir = 0;
    rc = snd_pcm_hw_params_set_rate_near(pcm_, params, &rate, &dir);
    if (rc < 0) { std::cerr << "set_rate: " << snd_strerror(rc) << "\n"; return false; }
    actualRate_ = rate;

    // Stable defaults on Pi: tune later if needed
    periodFrames_ = 512;
    bufferFrames_ = periodFrames_ * 4;
    snd_pcm_hw_params_set_period_size_near(pcm_, params, &periodFrames_, &dir);
    snd_pcm_hw_params_set_buffer_size_near(pcm_, params, &bufferFrames_);

    rc = snd_pcm_hw_params(pcm_, params);
    if (rc < 0) {
        std::cerr << "snd_pcm_hw_params: " << snd_strerror(rc) << "\n";
        return false;
    }

    snd_pcm_hw_params_get_period_size(params, &periodFrames_, &dir);
    snd_pcm_hw_params_get_buffer_size(params, &bufferFrames_);

    std::cout << "ALSA open: rate=" << actualRate_
              << " ch=" << channels_
              << " period=" << periodFrames_
              << " buffer=" << bufferFrames_ << "\n";
    return true;
}

void AlsaOut::close() {
    if (!pcm_) return;
    snd_pcm_drain(pcm_);
    snd_pcm_close(pcm_);
    pcm_ = nullptr;
}

void AlsaOut::writeFrames(const int16_t* data, snd_pcm_sframes_t frames) {
    if (!pcm_) return;
    while (frames > 0) {
        snd_pcm_sframes_t written = snd_pcm_writei(pcm_, data, frames);
        if (written == -EPIPE) {
            // underrun
            snd_pcm_prepare(pcm_);
            continue;
        } else if (written < 0) {
            std::cerr << "writei error: " << snd_strerror((int)written) << "\n";
            snd_pcm_prepare(pcm_);
            continue;
        }
        frames -= written;
        data += written * channels_;
    }
}

bool AlsaOut::pause(bool on) {
    if (!pcm_) return false;
    int rc = snd_pcm_pause(pcm_, on ? 1 : 0);
    if (rc == 0) return true;

    // If not supported, ALSA returns a negative error code (often -ENOSYS)
    std::cerr << "snd_pcm_pause not supported or failed: " << snd_strerror(rc) << "\n";
    return false;
}
