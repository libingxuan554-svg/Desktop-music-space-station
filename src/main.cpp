#include "Player.hpp"
#include "UI.hpp"
#include "PIR.hpp"
#include "LED.hpp"

#include <filesystem>
#include <iostream>
#include <vector>
#include <algorithm>

static std::vector<std::string> scanMusic(const std::string& dir)
{
    namespace fs = std::filesystem;
    std::vector<std::string> files;
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

int main()
{
    Player player;
    if (!player.init()) {
        std::cerr << "Player init failed\n";
        return 1;
    }

    auto files = scanMusic("./music");
    if (files.empty()) {
        std::cerr << "No music found in ./music (mp3/wav/flac)\n";
    }
    player.setPlaylist(files);
    if (!files.empty()) player.playIndex(0);

    LED led;
    // WS2812B: GPIO18, 30颗灯（按你实际灯珠数量改）
    if (!led.start(18, 30)) {
        std::cerr << "LED init failed (continuing without LED)\n";
    }

    PIR pir;
    // Pi 的 gpiochip 通常是 gpiochip0；line 17 表示 GPIO17（AM312 OUT 接这里）
    pir.start("gpiochip0", 17, [&](){
        // 运动触发：你可以改成“唤醒屏幕/亮度/自动播放”等逻辑
        std::cout << "[PIR] motion!\n";
        if (!player.isPlaying()) player.play();
    });

    UI ui;
    // 7寸常见 800x480，字体路径你要放 assets/font.ttf
    if (!ui.init(800, 480, "./assets/font.ttf")) {
        std::cerr << "UI init failed\n";
        pir.stop();
        led.stop();
        player.shutdown();
        return 1;
    }

    ui.run(player, [&](float lvl){
        led.setLevel(lvl);
    });

    ui.shutdown();
    pir.stop();
    led.stop();
    player.shutdown();
    return 0;
}
