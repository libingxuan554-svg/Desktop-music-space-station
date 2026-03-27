#include <iostream>
#include <string>
#include <vector>
#include <filesystem>
#include <algorithm>

#include "../SystemInterfaces.hpp"
#include "WavDecoder.hpp"
#include "RingBuffer.hpp"
#include "MusicController.hpp"
#include "AudioEngine.hpp"
#include "HardwareController.hpp"

namespace fs = std::filesystem;

int main() {
    HardwareController hw;
    hw.initialize();
    hw.initLedStrip();

    WavDecoder decoder;
    RingBuffer buffer(8192);
    MusicController controller(&decoder, &buffer);
    AudioEngine engine(&controller, &hw); 

    if (!engine.init("pulse", 44100)) return -1;
    engine.start();

    std::vector<std::string> playlist;
    for (const auto& entry : fs::directory_iterator("../assets")) {
        if (entry.path().extension() == ".wav") playlist.push_back(entry.path().string());
    }
    std::sort(playlist.begin(), playlist.end());
    controller.setPlaylist(playlist);

    std::cout << "🎵 空间站就绪。指令: p(暂停), n(下), b(上), s [0-1](跳转), q(退出)" << std::endl;

    std::string input;
    while (std::cout << ">> " && std::cin >> input) {
        System::ControlCommand cmd;
        if (input == "p") {
            cmd.type = System::CommandType::PLAY_PAUSE;
            controller.processCommand(cmd);
        } else if (input == "n") {
            cmd.type = System::CommandType::NEXT_TRACK;
            controller.processCommand(cmd);
        } else if (input == "b") {
            cmd.type = System::CommandType::PREV_TRACK;
            controller.processCommand(cmd);
        } else if (input == "s") {
            float val;
            if (std::cin >> val) {
                if (val > 1.0f) val /= 100.0f;
                cmd.type = System::CommandType::SEEK_FORWARD;
                cmd.floatValue = val;
                controller.processCommand(cmd);
            }
        } else if (input == "q") break;
    }

    engine.stop();
    hw.shutdown();
    return 0;
}
