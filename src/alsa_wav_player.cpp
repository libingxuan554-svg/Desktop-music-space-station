#include <alsa/asoundlib.h>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <string>
#include <vector>
#include <algorithm>

#pragma pack(push, 1)
struct WavHeader {
    char riff[4];        // "RIFF"
    uint32_t riffSize;
    char wave[4];        // "WAVE"

    char fmt[4];         // "fmt "
    uint32_t fmtSize;    // 16 for PCM
    uint16_t audioFormat; // 1 = PCM
    uint16_t numChannels;
    uint32_t sampleRate;
    uint32_t byteRate;
    uint16_t blockAlign;
    uint16_t bitsPerSample;

    char data[4];        // "data" (may not appear immediately if there are extra chunks)
    uint32_t dataSize;
};
#pragma pack(pop)

static bool readExact(FILE* f, void* buf, size_t n) {
    return std::fread(buf, 1, n, f) == n;
}

// WAV can contain extra chunks; this scans for "data"
static bool findDataChunk(FILE* f, WavHeader& h, uint32_t& dataSizeOut) {
    // We already read first 44 bytes into h, but "data" might not be at that offset if fmtSize != 16 or extra chunks exist.
    // So: rewind and parse chunks properly (simple robust scan).
    std::fseek(f, 0, SEEK_SET);

    char riff[4]; uint32_t riffSize; char wave[4];
    if (!readExact(f, riff, 4) || std::memcmp(riff, "RIFF", 4) != 0) return false;
    if (!readExact(f, &riffSize, 4)) return false;
    if (!readExact(f, wave, 4) || std::memcmp(wave, "WAVE", 4) != 0) return false;

    bool gotFmt = false, gotData = false;
    uint16_t audioFormat=0, numChannels=0, bitsPerSample=0;
    uint32_t sampleRate=0;

    while (!gotData) {
        char chunkId[4];
        uint32_t chunkSize;
        if (!readExact(f, chunkId, 4)) return false;
        if (!readExact(f, &chunkSize, 4)) return false;

        if (std::memcmp(chunkId, "fmt ", 4) == 0) {
            // Read at least first 16 bytes of fmt
            if (chunkSize < 16) return false;
            uint16_t af, ch, bps;
            uint32_t sr, br;
            uint16_t ba;
            if (!readExact(f, &af, 2)) return false;
            if (!readExact(f, &ch, 2)) return false;
            if (!readExact(f, &sr, 4)) return false;
            if (!readExact(f, &br, 4)) return false;
            if (!readExact(f, &ba, 2)) return false;
            if (!readExact(f, &bps, 2)) return false;

            // Skip any remaining fmt bytes
            long remaining = (long)chunkSize - 16;
            if (remaining > 0) std::fseek(f, remaining, SEEK_CUR);

            audioFormat = af; numChannels = ch; sampleRate = sr; bitsPerSample = bps;
            gotFmt = true;
        } else if (std::memcmp(chunkId, "data", 4) == 0) {
            dataSizeOut = chunkSize;
            gotData = true;
            break; // file pointer is now at start of PCM data
        } else {
            // Skip unknown chunk (plus padding to even boundary if needed)
            std::fseek(f, chunkSize, SEEK_CUR);
        }

        // Chunks are word-aligned
        if (chunkSize & 1) std::fseek(f, 1, SEEK_CUR);
    }

    if (!gotFmt || !gotData) return false;

    // Fill a minimal header struct for reporting
    std::memcpy(h.riff, "RIFF", 4);
    std::memcpy(h.wave, "WAVE", 4);
    h.audioFormat = audioFormat;
    h.numChannels = numChannels;
    h.sampleRate = sampleRate;
    h.bitsPerSample = bitsPerSample;
    h.dataSize = dataSizeOut;
    return true;
}

static int16_t clamp16(int32_t x) {
    if (x > 32767) return 32767;
    if (x < -32768) return -32768;
    return (int16_t)x;
}

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <file.wav> [volume0to1]\n";
        return 1;
    }
    std::string path = argv[1];
    float volume = 1.0f;
    if (argc >= 3) volume = std::clamp(std::stof(argv[2]), 0.0f, 1.0f);

    FILE* f = std::fopen(path.c_str(), "rb");
    if (!f) { std::perror("fopen"); return 1; }

    WavHeader h{};
    uint32_t dataSize = 0;
    if (!findDataChunk(f, h, dataSize)) {
        std::cerr << "Not a valid WAV or failed to find fmt/data chunks.\n";
        std::fclose(f);
        return 1;
    }

    if (h.audioFormat != 1) {
        std::cerr << "Only PCM WAV supported. audioFormat=" << h.audioFormat << "\n";
        std::fclose(f);
        return 1;
    }
    if (h.bitsPerSample != 16) {
        std::cerr << "Only 16-bit PCM supported. bitsPerSample=" << h.bitsPerSample << "\n";
        std::fclose(f);
        return 1;
    }

    std::cout << "WAV: " << h.sampleRate << " Hz, " << h.numChannels << " ch, 16-bit, data " << dataSize << " bytes\n";

    // ---- ALSA open/config ----
    snd_pcm_t* pcm = nullptr;
    int rc = snd_pcm_open(&pcm, "default", SND_PCM_STREAM_PLAYBACK, 0);
    if (rc < 0) {
        std::cerr << "snd_pcm_open: " << snd_strerror(rc) << "\n";
        std::fclose(f);
        return 1;
    }

    snd_pcm_hw_params_t* params = nullptr;
    snd_pcm_hw_params_alloca(&params);
    snd_pcm_hw_params_any(pcm, params);

    rc = snd_pcm_hw_params_set_access(pcm, params, SND_PCM_ACCESS_RW_INTERLEAVED);
    if (rc < 0) { std::cerr << "set_access: " << snd_strerror(rc) << "\n"; return 1; }

    rc = snd_pcm_hw_params_set_format(pcm, params, SND_PCM_FORMAT_S16_LE);
    if (rc < 0) { std::cerr << "set_format: " << snd_strerror(rc) << "\n"; return 1; }

    rc = snd_pcm_hw_params_set_channels(pcm, params, h.numChannels);
    if (rc < 0) { std::cerr << "set_channels: " << snd_strerror(rc) << "\n"; return 1; }

    unsigned int rate = h.sampleRate;
    int dir = 0;
    rc = snd_pcm_hw_params_set_rate_near(pcm, params, &rate, &dir);
    if (rc < 0) { std::cerr << "set_rate: " << snd_strerror(rc) << "\n"; return 1; }

    // Buffer/period: keep simple but stable on Pi
    // period ~ 256~1024 frames, buffer ~ 4*period
    snd_pcm_uframes_t periodFrames = 512;
    snd_pcm_uframes_t bufferFrames = periodFrames * 4;

    snd_pcm_hw_params_set_period_size_near(pcm, params, &periodFrames, &dir);
    snd_pcm_hw_params_set_buffer_size_near(pcm, params, &bufferFrames);

    rc = snd_pcm_hw_params(pcm, params);
    if (rc < 0) {
        std::cerr << "snd_pcm_hw_params: " << snd_strerror(rc) << "\n";
        snd_pcm_close(pcm);
        std::fclose(f);
        return 1;
    }

    snd_pcm_hw_params_get_period_size(params, &periodFrames, &dir);
    snd_pcm_hw_params_get_buffer_size(params, &bufferFrames);

    std::cout << "ALSA configured rate=" << rate
              << " periodFrames=" << periodFrames
              << " bufferFrames=" << bufferFrames << "\n";

    // ---- playback loop ----
    const size_t frameBytes = (h.numChannels * 2); // 16-bit = 2 bytes
    const size_t chunkBytes = (size_t)periodFrames * frameBytes;

    std::vector<int16_t> buffer((chunkBytes / 2), 0); // int16 samples
    size_t bytesLeft = dataSize;

    while (bytesLeft > 0) {
        size_t toRead = std::min(chunkBytes, bytesLeft);
        size_t got = std::fread(buffer.data(), 1, toRead, f);
        if (got == 0) break;

        // software volume scaling
        if (volume < 0.999f) {
            int16_t* s = buffer.data();
            size_t samples = got / 2;
            for (size_t i = 0; i < samples; i++) {
                int32_t v = (int32_t)std::lround((float)s[i] * volume);
                s[i] = clamp16(v);
            }
        }

        snd_pcm_sframes_t framesToWrite = (snd_pcm_sframes_t)(got / frameBytes);
        int16_t* ptr = buffer.data();

        while (framesToWrite > 0) {
            snd_pcm_sframes_t written = snd_pcm_writei(pcm, ptr, framesToWrite);
            if (written == -EPIPE) {
                // underrun
                snd_pcm_prepare(pcm);
                continue;
            } else if (written < 0) {
                std::cerr << "writei error: " << snd_strerror((int)written) << "\n";
                snd_pcm_prepare(pcm);
                continue;
            }
            framesToWrite -= written;
            ptr += written * h.numChannels;
        }

        bytesLeft -= got;
    }

    snd_pcm_drain(pcm);
    snd_pcm_close(pcm);
    std::fclose(f);
    return 0;
}
