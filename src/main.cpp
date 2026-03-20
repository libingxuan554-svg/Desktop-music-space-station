#include "Player.hpp"
#include "LED.hpp"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <filesystem>
#include <iostream>
#include <string>
#include <thread>
#include <vector>

static std::vector<std::string> scanMusic(const std::string& dir)
{
    namespace fs = std::filesystem;
    std::vector<std::string> files;

    if (!fs::exists(dir)) {
        std::cerr << "[WARN] Music directory not found: " << dir << "\n";
        return files;
    }

    for (auto& p : fs::directory_iterator(dir)) {
        if (!p.is_regular_file()) continue;

        auto ext = p.path().extension().string();
        std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

        if (ext == ".mp3" || ext == ".wav" || ext == ".flac") {
            files.push_back(p.path().string());
        }
    }

    std::sort(files.begin(), files.end());
    return files;
}

static void printHelp()
{
    std::cout
        << "\nCommands:\n"
        << "  p            : toggle play/pause\n"
        << "  n            : next track\n"
        << "  b            : previous track\n"
        << "  + / -        : volume up/down (0..1)\n"
        << "  s <seconds>  : seek to position (seconds)\n"
        << "  i            : info (title/index/pos/dur/vol)\n"
        << "  q            : quit\n\n";
}

int main()
{
    // 1) Init player
    Player player;
    if (!player.init()) {
        std::cerr << "Player init failed\n";
        return 1;
    }

    // 2) Load playlist
    auto files = scanMusic("./music");
    if (files.empty()) {
        std::cerr << "No music found in ./music (mp3/wav/flac)\n";
        std::cerr << "Put audio files into ./music and run again.\n";
        player.shutdown();
        return 1;
    }

    player.setPlaylist(files);
    player.playIndex(0);

    // 3) Init LED (optional)
    LED led;
    bool led_ok = led.start(18, 30); // GPIO18, 30 LEDs (change to your strip length)
    if (!led_ok) {
        std::cerr << "[WARN] LED init failed (continuing without LED)\n";
    }

    // 4) LED level updater thread (music reactive)
    std::atomic<bool> running{true};
    std::thread ledThread([&]() {
        using namespace std::chrono_literals;
        while (running.load()) {
            if (led_ok) {
                // Player already provides 0..1 energy estimate for LEDs
                led.setLevel(player.level01());
            }
            std::this_thread::sleep_for(20ms); // ~50 FPS
        }
    });

    // 5) Simple CLI control loop
    printHelp();
    std::string line;

    while (true) {
        std::cout << "> ";
        if (!std::getline(std::cin, line)) break;

        // trim leading spaces
        auto first = line.find_first_not_of(" \t");
        if (first == std::string::npos) continue;
        line = line.substr(first);

        if (line == "q") {
            break;
        } else if (line == "h" || line == "help") {
            printHelp();
        } else if (line == "p") {
            player.toggle();
        } else if (line == "n") {
            player.next();
        } else if (line == "b") {
            player.prev();
        } else if (line == "+") {
            float v = player.volume();
            v = std::min(1.0f, v + 0.05f);
            player.setVolume(v);
            std::cout << "Volume: " << v << "\n";
        } else if (line == "-") {
            float v = player.volume();
            v = std::max(0.0f, v - 0.05f);
            player.setVolume(v);
            std::cout << "Volume: " << v << "\n";
        } else if (line.size() >= 2 && line[0] == 's' && std::isspace((unsigned char)line[1])) {
            try {
                double sec = std::stod(line.substr(2));
                player.seekSeconds(sec);
            } catch (...) {
                std::cout << "Usage: s <seconds>\n";
            }
        } else if (line == "i") {
            std::cout
                << "Title: " << player.currentTitle() << "\n"
                << "Index: " << player.currentIndex() << " / " << (int)player.playlist().size() - 1 << "\n"
                << "Pos  : " << player.positionSeconds() << " / " << player.durationSeconds() << " sec\n"
                << "Vol  : " << player.volume() << "\n"
                << "Play : " << (player.isPlaying() ? "yes" : "no") << "\n";
        } else {
            std::cout << "Unknown command. Type 'h' for help.\n";
        }
    }

    // 6) Shutdown
    running.store(false);
    if (ledThread.joinable()) ledThread.join();

    if (led_ok) led.stop();
    player.shutdown();

    return 0;
}
