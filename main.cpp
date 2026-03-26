#include "AudioEngine.h"
#include <iostream>
#include <thread>
#include <chrono>

int main() {
    AudioEngine audio;

    if (!audio.init(48000, 2, 2048, 2)) {
        std::cerr << "Audio init failed.\n";
        return -1;
    }

    if (!audio.load("test.mp3")) {
        std::cerr << "Load file failed.\n";
        return -1;
    }

    audio.setVolume(0.8f);
    audio.play();

    for (int i = 0; i < 200; ++i) {
        std::vector<float> chunk;
        if (audio.getPcmChunk(chunk)) {
            std::cout << "PCM chunk received, samples = " << chunk.size() << std::endl;
        } else {
            std::cout << "PCM chunk not enough yet." << std::endl;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }

    audio.stop();
    return 0;
}
