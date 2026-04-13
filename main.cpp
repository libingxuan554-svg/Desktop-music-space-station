#include <iostream>
#include <string>
#include <vector>
#include <filesystem>
#include <algorithm>
#include <thread>
#include <chrono>
#include <atomic>
#include <cmath> 
#include <numeric> 
#include <sys/timerfd.h> // Linux 内核定时器
#include <unistd.h>
#include <mutex>
#include <condition_variable>
#include <queue>

#include "SystemInterfaces.hpp"
#include "WavDecoder.hpp"
#include "RingBuffer.hpp"
#include "MusicController.hpp"
#include "AudioEngine.hpp"
#include "HardwareController.hpp"
#include "MediaProgressManager.hpp" 
#include "FramebufferUI.hpp"
#include "TouchHandler.hpp"
#include "InteractionManager.hpp"
#include "EnvironmentMonitor.hpp"

namespace fs = std::filesystem;

int main() {
    HardwareController hw;
    hw.initialize();
    hw.initLedStrip();

    WavDecoder decoder;
    RingBuffer buffer(8192);
    MusicController controller(&decoder, &buffer);
    AudioEngine engine(&controller, &hw);

    MediaProgressManager progressMgr(&decoder);

    // 启动数据采集
    EnvironmentMonitor envMonitor;
    envMonitor.start();

    //
    if (engine.init("plughw:0,0", 44100)) {
        engine.start();
        std::cout << "ALSA Engine Started." << std::endl;
    }

    std::vector<std::string> playlist;
    if (fs::exists("../assets")) {
        for (const auto& entry : fs::directory_iterator("../assets")) {
            if (entry.path().extension() == ".wav") {
                playlist.push_back(entry.path().string());
            }
        }
    }
    std::sort(playlist.begin(), playlist.end());
    controller.setPlaylist(playlist);

    UI::FramebufferUI fbUi("/dev/fb0");
    UI::TouchHandler touch("/dev/input/event0");

    fbUi.preloadBmp("STANDBY", "../UI/Renfer/bg_standy_clean_1024x600.bmp");
    fbUi.preloadBmp("MUSIC_LIST", "../UI/Renfer/bg_music_list_clean_1024x600.bmp");
    fbUi.preloadBmp("PLAYER", "../UI/Renfer/bg_music_player_clean_1024x600.bmp");

    std::queue<System::ControlCommand> cmdQueue;
    std::mutex cmdMutex;
    std::condition_variable cmdCV;
    std::atomic<bool> uiRunning{true};

    // 消费者线程：指令处理
    std::thread cmdWorker([&]() {
        while (true) {
            System::ControlCommand cmd;
            {
                std::unique_lock<std::mutex> lock(cmdMutex);
                cmdCV.wait(lock, [&]() { return !cmdQueue.empty() || !uiRunning; });
                if (!uiRunning && cmdQueue.empty()) break;
                cmd = cmdQueue.front();
                cmdQueue.pop();
            }
            if (cmd.type == System::CommandType::ENTER_STANDBY) {
                if (controller.isPlaying()) {
                    System::ControlCommand p; p.type = System::CommandType::PLAY_PAUSE;
                    controller.processCommand(p);
                }
            } else {
                controller.processCommand(cmd);
            }
        }
    });

    UI::InteractionManager::setCommandEmitter([&](const System::ControlCommand& cmd) {
        std::lock_guard<std::mutex> lock(cmdMutex);
        cmdQueue.push(cmd);
        cmdCV.notify_one();
    });

    touch.startListening(
        [](int x, int y) { UI::InteractionManager::handleTouch(x, y); },
        [](int deltaY) { UI::InteractionManager::handleScroll(deltaY); }
    );

    // UI 渲染主线程
    std::thread uiThread([&]() {
        System::PlaybackStatus status;
        std::vector<std::string> displayNames;
        for (const auto& pathStr : playlist) {
            displayNames.push_back(fs::path(pathStr).stem().string());
        }
        status.playlist = displayNames;

        // 创建高精度内核定时器 (33.3ms = 30FPS)
        int timer_fd = timerfd_create(CLOCK_MONOTONIC, 0);
        if (timer_fd == -1) {
            std::cerr << "FATAL ERROR: Failed to create hardware timer!" << std::endl;
            exit(EXIT_FAILURE);
        }
        
        struct itimerspec its;
        its.it_value.tv_sec = 0;
        its.it_value.tv_nsec = 33333333;
        its.it_interval.tv_sec = 0;
        its.it_interval.tv_nsec = 33333333;
        timerfd_settime(timer_fd, 0, &its, NULL);

        while (uiRunning) {
            progressMgr.injectTimeData(status);
            status.isPlaying = controller.isPlaying();
            status.volume = static_cast<int32_t>(controller.getVolume() * 100.0f);

            int currentIndex = controller.getCurrentTrackIndex();
            if (currentIndex >= 0 && (size_t)currentIndex < displayNames.size()) {
                status.songName = displayNames[currentIndex];
            }

            // 自动连播逻辑
            if (status.isPlaying && status.totalDuration > 0) {
                if (status.currentPosition >= status.totalDuration) {
                    System::ControlCommand nextCmd;
                    nextCmd.type = System::CommandType::NEXT_TRACK;
                    {
                        std::lock_guard<std::mutex> lock(cmdMutex);
                        cmdQueue.push(nextCmd);
                        cmdCV.notify_one();
                    }
                }
            }

            // 频谱能量计算
            System::AudioVisualData visual;
            if (status.isPlaying) {
                std::vector<float> spectrum = hw.getCurrentSpectrum();
                if (!spectrum.empty()) {
                    float sum = std::accumulate(spectrum.begin(), spectrum.end(), 0.0f);
                    visual.overallIntensity = std::clamp((sum / spectrum.size()) * 2.0f, 0.0f, 1.0f);
                }
            } else {
                visual.overallIntensity = 0.0f;
            }

            System::EnvironmentStatus env = envMonitor.getLatestStatus();
            UI::InteractionManager::updateSystemStatus(status, visual);
            UI::InteractionManager::updateEnvStatus(env);
            UI::InteractionManager::renderCurrentPage(&fbUi);

            // 阻塞等待硬件定时器，彻底消除 Poll
            if (timer_fd != -1) {
                uint64_t missed;
                read(timer_fd, &missed, sizeof(missed)); 
            }
        } // <---  修复点：正确闭合 while 循环

        if (timer_fd != -1) close(timer_fd);
    });

    // 终端输入循环
    std::string input;
    while (uiRunning && std::cout << ">> " && std::cin >> input) {
        System::ControlCommand cmd;
        bool valid = true;
        if (input == "p") cmd.type = System::CommandType::PLAY_PAUSE;
        else if (input == "n") cmd.type = System::CommandType::NEXT_TRACK;
        else if (input == "b") cmd.type = System::CommandType::PREV_TRACK;
        else if (input == "q") { uiRunning = false; break; }
        else valid = false;

        if (valid) {
            std::lock_guard<std::mutex> lock(cmdMutex);
            cmdQueue.push(cmd);
            cmdCV.notify_one();
        }
    }

    // 安全退出序列
    uiRunning = false;
    cmdCV.notify_all(); 
    if (cmdWorker.joinable()) cmdWorker.join();
    if (uiThread.joinable()) uiThread.join();

    touch.stopListening();
    engine.stop();
    hw.shutdown();
    envMonitor.stop();
    return 0;
}
// <---  修复点：删除了多余的花括号
