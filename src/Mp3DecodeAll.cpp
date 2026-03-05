#include "Mp3DecodeAll.h"
#include <cstring>
#include <iostream>

// minimp3
#define MINIMP3_IMPLEMENTATION
#define MINIMP3_EX_IMPLEMENTATION
#include "../third_party/minimp3/minimp3.h"
#include "../third_party/minimp3/minimp3_ex.h"

bool decodeMp3All(const std::string& path, DecodedPcm& out) {
    mp3dec_t mp3d;
    mp3dec_file_info_t info;
    std::memset(&info, 0, sizeof(info));
    mp3dec_init(&mp3d);

    int rc = mp3dec_load(&mp3d, path.c_str(), &info, NULL, NULL);
    if (rc) {
        std::cerr << "mp3dec_load failed rc=" << rc << "\n";
        return false;
    }
    if (!info.buffer || info.samples == 0) {
        std::cerr << "decoded buffer empty\n";
        mp3dec_file_info_free(&info);
        return false;
    }

    out.sampleRate = info.hz;
    out.channels = info.channels;

    out.samples.assign((int16_t*)info.buffer, (int16_t*)info.buffer + info.samples);

    mp3dec_file_info_free(&info);
    return true;
}
