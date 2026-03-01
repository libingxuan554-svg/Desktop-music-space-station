#pragma once
#include <cstdint>
#include <string>
#include <vector>

struct DecodedPcm {
    int sampleRate = 0;
    int channels = 0;
    std::vector<int16_t> samples; // interleaved, int16
};

bool decodeMp3All(const std::string& path, DecodedPcm& out);
