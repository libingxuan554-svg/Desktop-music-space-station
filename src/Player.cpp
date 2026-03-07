#define MINIAUDIO_IMPLEMENTATION
#define MA_NO_DEVICE_IO
#include "miniaudio.h"

#include "Player.hpp"
#include <algorithm>
#include <filesystem>
#include <iostream>

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

    Callback onChanged;

    ma_uint64 framesPlayed = 0;
    ma_uint32 channels = 2;
    ma_uint32 sampleRate = 48000;

    bool decoderReady = false;
};

static void data_callback(ma_device* pDevice, void* pOutput, const void*, ma_uint32 frameCount)
{
    auto* self = (Player::Impl*)pDevice->pUserData;
    float* out = (float*)pOutput;

    std::lock_guard<std::mutex> lk(self->mtx);

    if (!self->playing.load() || !self->decoderReady) {
        std::fill(out, out + frameCount * self->channels, 0.0f);
        self->level.store(0.0f);
        return;
    }

    ma_uint64 framesRead = 0;
    ma_result r = ma_decoder_read_pcm_frames(&self->decoder, out, frameCount, &framesRead);
    if (r != MA_SUCCESS || framesRead == 0) {
        // 曲终：自动停（UI 可调用 next）
        std::fill(out, out + frameCount * self->channels, 0.0f);
        self->playing.store(false);
        self->level.store(0.0f);
        return;
    }

    // 音量 & 简单电平估计（RMS-like）
    float v = self->vol.load();
    double acc = 0.0;
    const ma_uint64 n = framesRead * self->channels;
    for (ma_uint64 i = 0; i < n; ++i) {
        out[i] *= v;
        acc += (double)out[i] * (double)out[i];
    }
    float rms = (n > 0) ? (float)std::sqrt(acc / (double)n) : 0.0f;
    // 压一下范围，方便驱动 LED
    float lvl = std::min(1.0f, rms * 6.0f);
    self->level.store(lvl);

    self->framesPlayed += framesRead;
    double pos = (double)self->framesPlayed / (double)self->sampleRate;
    self->posSec.store(pos);
}

bool Player::init()
{
    impl_ = new Impl();

    if (ma_context_init(nullptr, 0, nullptr, &impl_->ctx) != MA_SUCCESS) {
        std::cerr << "ma_context_init failed\n";
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
        return false;
    }

    impl_->channels = cfg.playback.channels;
    impl_->sampleRate = cfg.sampleRate;

    if (ma_device_start(&impl_->device) != MA_SUCCESS) {
        std::cerr << "ma_device_start failed\n";
        return false;
    }

    return true;
}

void Player::shutdown()
{
    if (!impl_) return;
    {
        std::lock_guard<std::mutex> lk(impl_->mtx);
        if (impl_->decoderReady) {
            ma_decoder_uninit(&impl_->decoder);
            impl_->decoderReady = false;
        }
    }
    ma_device_uninit(&impl_->device);
    ma_context_uninit(&impl_->ctx);
    delete impl_;
    impl_ = nullptr;
}

void Player::setPlaylist(std::vector<std::string> files)
{
    impl_->files = std::move(files);
    if (impl_->onChanged) impl_->onChanged();
}
const std::vector<std::string>& Player::playlist() const { return impl_->files; }

static bool openDecoder(Player::Impl* impl, const std::string& path)
{
    if (impl->decoderReady) {
        ma_decoder_uninit(&impl->decoder);
        impl->decoderReady = false;
    }

    ma_decoder_config dcfg = ma_decoder_config_init(ma_format_f32, 2, 48000);
    if (ma_decoder_init_file(path.c_str(), &dcfg, &impl->decoder) != MA_SUCCESS) {
        std::cerr << "decoder init failed: " << path << "\n";
        return false;
    }
    impl->decoderReady = true;

    impl->framesPlayed = 0;
    impl->posSec.store(0.0);

    ma_uint64 totalFrames = 0;
    if (ma_decoder_get_length_in_pcm_frames(&impl->decoder, &totalFrames) == MA_SUCCESS) {
        impl->durSec.store((double)totalFrames / (double)impl->sampleRate);
    } else {
        impl->durSec.store(0.0);
    }

    return true;
}

bool Player::playIndex(int idx)
{
    if (idx < 0 || idx >= (int)impl_->files.size()) return false;
    std::lock_guard<std::mutex> lk(impl_->mtx);

    if (!openDecoder(impl_, impl_->files[idx])) return false;
    impl_->idx.store(idx);
    impl_->playing.store(true);
    if (impl_->onChanged) impl_->onChanged();
    return true;
}

void Player::play()  { impl_->playing.store(true);  if (impl_->onChanged) impl_->onChanged(); }
void Player::pause() { impl_->playing.store(false); if (impl_->onChanged) impl_->onChanged(); }
void Player::toggle(){ impl_->playing.store(!impl_->playing.load()); if (impl_->onChanged) impl_->onChanged(); }
void Player::stop()  { impl_->playing.store(false); seekSeconds(0); if (impl_->onChanged) impl_->onChanged(); }

void Player::next()
{
    int n = (int)impl_->files.size();
    if (n == 0) return;
    int i = impl_->idx.load();
    int ni = (i < 0) ? 0 : (i + 1) % n;
    playIndex(ni);
}
void Player::prev()
{
    int n = (int)impl_->files.size();
    if (n == 0) return;
    int i = impl_->idx.load();
    int pi = (i <= 0) ? (n - 1) : (i - 1);
    playIndex(pi);
}

void Player::seekSeconds(double sec)
{
    std::lock_guard<std::mutex> lk(impl_->mtx);
    if (!impl_->decoderReady) return;

    sec = std::max(0.0, sec);
    ma_uint64 frame = (ma_uint64)(sec * (double)impl_->sampleRate);
    ma_decoder_seek_to_pcm_frame(&impl_->decoder, frame);
    impl_->framesPlayed = frame;
    impl_->posSec.store(sec);
}

void Player::setVolume(float v01)
{
    v01 = std::max(0.0f, std::min(1.0f, v01));
    impl_->vol.store(v01);
    if (impl_->onChanged) impl_->onChanged();
}
float Player::volume() const { return impl_->vol.load(); }

bool Player::isPlaying() const { return impl_->playing.load(); }
int  Player::currentIndex() const { return impl_->idx.load(); }
double Player::positionSeconds() const { return impl_->posSec.load(); }
double Player::durationSeconds() const { return impl_->durSec.load(); }
float Player::level01() const { return impl_->level.load(); }

void Player::setOnStateChanged(Callback cb) { impl_->onChanged = std::move(cb); }

std::string Player::currentTitle() const
{
    int i = impl_->idx.load();
    if (i < 0 || i >= (int)impl_->files.size()) return "(no track)";
    return std::filesystem::path(impl_->files[i]).filename().string();
}
