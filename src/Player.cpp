#define MINIAUDIO_IMPLEMENTATION
#define MA_NO_DEVICE_IO
#include "miniaudio.h"

#include "Player.hpp"

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <iostream>
#include <mutex>
#include <atomic>

struct Player::Impl {
    ma_context ctx{};
    ma_device  device{};
    ma_decoder decoder{};
    std::mutex mtx;

    std::vector<std::string> files;

    std::atomic<int> idx{-1};
    std::atomic<bool> playing{false};
    std::atomic<float> vol{0.8f};

    std::atomic<double> posSec{0.0};
    std::atomic<double> durSec{0.0};

    std::atomic<float> level{0.0f};

    std::atomic<bool> initialized{false};
    std::atomic<bool> decoderReady{false};
    std::atomic<bool> trackEnded{false};

    Callback onChanged;

    ma_uint64 framesPlayed = 0;
    ma_uint32 channels = 2;
    ma_uint32 sampleRate = 48000;
};

static bool openDecoder(Player::Impl* impl, const std::string& path)
{
    if (!impl) return false;

    if (impl->decoderReady.load()) {
        ma_decoder_uninit(&impl->decoder);
        impl->decoderReady.store(false);
    }

    ma_decoder_config dcfg = ma_decoder_config_init(ma_format_f32, 2, 48000);
    if (ma_decoder_init_file(path.c_str(), &dcfg, &impl->decoder) != MA_SUCCESS) {
        std::cerr << "decoder init failed: " << path << "\n";
        return false;
    }

    impl->decoderReady.store(true);
    impl->framesPlayed = 0;
    impl->posSec.store(0.0);
    impl->trackEnded.store(false);
    impl->level.store(0.0f);

    ma_uint64 totalFrames = 0;
    if (ma_decoder_get_length_in_pcm_frames(&impl->decoder, &totalFrames) == MA_SUCCESS) {
        impl->durSec.store(static_cast<double>(totalFrames) / static_cast<double>(impl->sampleRate));
    } else {
        impl->durSec.store(0.0);
    }

    return true;
}

static void data_callback(ma_device* pDevice, void* pOutput, const void*, ma_uint32 frameCount)
{
    auto* self = static_cast<Player::Impl*>(pDevice->pUserData);
    float* out = static_cast<float*>(pOutput);

    if (!self) return;

    std::lock_guard<std::mutex> lk(self->mtx);

    const ma_uint64 sampleCount = static_cast<ma_uint64>(frameCount) * self->channels;

    if (!self->playing.load() || !self->decoderReady.load()) {
        std::fill(out, out + sampleCount, 0.0f);
        self->level.store(0.0f);
        return;
    }

    ma_uint64 framesRead = 0;
    ma_result r = ma_decoder_read_pcm_frames(&self->decoder, out, frameCount, &framesRead);

    if (r != MA_SUCCESS || framesRead == 0) {
        std::fill(out, out + sampleCount, 0.0f);
        self->playing.store(false);
        self->level.store(0.0f);
        self->trackEnded.store(true);
        return;
    }

    if (framesRead < frameCount) {
        std::fill(out + framesRead * self->channels, out + sampleCount, 0.0f);
        self->trackEnded.store(true);
        self->playing.store(false);
    }

    float v = self->vol.load();
    double acc = 0.0;
    const ma_uint64 n = framesRead * self->channels;

    for (ma_uint64 i = 0; i < n; ++i) {
        out[i] *= v;
        acc += static_cast<double>(out[i]) * static_cast<double>(out[i]);
    }

    float rms = (n > 0) ? static_cast<float>(std::sqrt(acc / static_cast<double>(n))) : 0.0f;
    float lvl = std::min(1.0f, rms * 6.0f);
    self->level.store(lvl);

    self->framesPlayed += framesRead;
    self->posSec.store(static_cast<double>(self->framesPlayed) / static_cast<double>(self->sampleRate));
}

Player::Player() : impl_(nullptr) {}

Player::~Player()
{
    shutdown();
}

bool Player::init()
{
    if (impl_) return true;

    impl_ = new Impl();

    if (ma_context_init(nullptr, 0, nullptr, &impl_->ctx) != MA_SUCCESS) {
        std::cerr << "ma_context_init failed\n";
        delete impl_;
        impl_ = nullptr;
        return false;
    }

    ma_device_config cfg = ma_device_config_init(ma_device_type_playback);
    cfg.playback.format   = ma_format_f32;
    cfg.playback.channels = 2;
    cfg.sampleRate        = 48000;
    cfg.dataCallback      = data_callback;
    cfg.pUserData         = impl_;

    if (ma_device_init(&impl_->ctx, &cfg, &impl_->device) != MA_SUCCESS) {
        std::cerr << "ma_device_init failed\n";
        ma_context_uninit(&impl_->ctx);
        delete impl_;
        impl_ = nullptr;
        return false;
    }

    impl_->channels = cfg.playback.channels;
    impl_->sampleRate = cfg.sampleRate;

    if (ma_device_start(&impl_->device) != MA_SUCCESS) {
        std::cerr << "ma_device_start failed\n";
        ma_device_uninit(&impl_->device);
        ma_context_uninit(&impl_->ctx);
        delete impl_;
        impl_ = nullptr;
        return false;
    }

    impl_->initialized.store(true);
    return true;
}

void Player::shutdown()
{
    if (!impl_) return;

    {
        std::lock_guard<std::mutex> lk(impl_->mtx);
        impl_->playing.store(false);

        if (impl_->decoderReady.load()) {
            ma_decoder_uninit(&impl_->decoder);
            impl_->decoderReady.store(false);
        }
    }

    if (impl_->initialized.load()) {
        ma_device_uninit(&impl_->device);
        ma_context_uninit(&impl_->ctx);
        impl_->initialized.store(false);
    }

    delete impl_;
    impl_ = nullptr;
}

void Player::setPlaylist(std::vector<std::string> files)
{
    if (!impl_) return;
    impl_->files = std::move(files);

    if (impl_->idx.load() >= static_cast<int>(impl_->files.size())) {
        impl_->idx.store(-1);
    }

    if (impl_->onChanged) impl_->onChanged();
}

const std::vector<std::string>& Player::playlist() const
{
    return impl_->files;
}

bool Player::playIndex(int idx)
{
    if (!impl_) return false;
    if (idx < 0 || idx >= static_cast<int>(impl_->files.size())) return false;

    {
        std::lock_guard<std::mutex> lk(impl_->mtx);
        if (!openDecoder(impl_, impl_->files[idx])) {
            return false;
        }

        impl_->idx.store(idx);
        impl_->playing.store(true);
        impl_->trackEnded.store(false);
    }

    if (impl_->onChanged) impl_->onChanged();
    return true;
}

void Player::play()
{
    if (!impl_) return;
    if (!impl_->decoderReady.load() && !impl_->files.empty() && impl_->idx.load() < 0) {
        playIndex(0);
        return;
    }

    impl_->playing.store(true);
    if (impl_->onChanged) impl_->onChanged();
}

void Player::pause()
{
    if (!impl_) return;
    impl_->playing.store(false);
    if (impl_->onChanged) impl_->onChanged();
}

void Player::toggle()
{
    if (!impl_) return;
    impl_->playing.store(!impl_->playing.load());
    if (impl_->onChanged) impl_->onChanged();
}

void Player::stop()
{
    if (!impl_) return;

    impl_->playing.store(false);
    seekSeconds(0.0);
    impl_->level.store(0.0f);
    impl_->trackEnded.store(false);

    if (impl_->onChanged) impl_->onChanged();
}

void Player::next()
{
    if (!impl_) return;
    int n = static_cast<int>(impl_->files.size());
    if (n == 0) return;

    int i = impl_->idx.load();
    int ni = (i < 0) ? 0 : (i + 1) % n;
    playIndex(ni);
}

void Player::prev()
{
    if (!impl_) return;
    int n = static_cast<int>(impl_->files.size());
    if (n == 0) return;

    int i = impl_->idx.load();
    int pi = (i <= 0) ? (n - 1) : (i - 1);
    playIndex(pi);
}

void Player::seekSeconds(double sec)
{
    if (!impl_) return;

    std::lock_guard<std::mutex> lk(impl_->mtx);
    if (!impl_->decoderReady.load()) return;

    sec = std::max(0.0, sec);

    double dur = impl_->durSec.load();
    if (dur > 0.0) sec = std::min(sec, dur);

    ma_uint64 frame = static_cast<ma_uint64>(sec * static_cast<double>(impl_->sampleRate));
    if (ma_decoder_seek_to_pcm_frame(&impl_->decoder, frame) == MA_SUCCESS) {
        impl_->framesPlayed = frame;
        impl_->posSec.store(sec);
        impl_->trackEnded.store(false);
    }
}

void Player::setVolume(float v01)
{
    if (!impl_) return;
    v01 = std::max(0.0f, std::min(1.0f, v01));
    impl_->vol.store(v01);

    if (impl_->onChanged) impl_->onChanged();
}

float Player::volume() const
{
    return impl_ ? impl_->vol.load() : 0.0f;
}

bool Player::isPlaying() const
{
    return impl_ ? impl_->playing.load() : false;
}

int Player::currentIndex() const
{
    return impl_ ? impl_->idx.load() : -1;
}

double Player::positionSeconds() const
{
    return impl_ ? impl_->posSec.load() : 0.0;
}

double Player::durationSeconds() const
{
    return impl_ ? impl_->durSec.load() : 0.0;
}

float Player::level01() const
{
    return impl_ ? impl_->level.load() : 0.0f;
}

void Player::update()
{
    if (!impl_) return;

    if (impl_->trackEnded.exchange(false)) {
        int n = static_cast<int>(impl_->files.size());
        if (n > 0) {
            next();
        } else {
            impl_->playing.store(false);
            impl_->level.store(0.0f);
            if (impl_->onChanged) impl_->onChanged();
        }
    }
}

void Player::setOnStateChanged(Callback cb)
{
    if (!impl_) return;
    impl_->onChanged = std::move(cb);
}

std::string Player::currentTitle() const
{
    if (!impl_) return "(no track)";

    int i = impl_->idx.load();
    if (i < 0 || i >= static_cast<int>(impl_->files.size())) {
        return "(no track)";
    }

    return std::filesystem::path(impl_->files[i]).filename().string();
}
