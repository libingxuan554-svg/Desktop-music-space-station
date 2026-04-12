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
#include <sys/timerfd.h> //linux 内核定时器
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

    //启动数据采集
    EnvironmentMonitor envMonitor;
    envMonitor.start();

    if (engine.init("plughw:0,0", 44100)) {
        engine.start();
        std::cout << "✅ ALSA Engine Started." << std::endl;
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
    std::atomic<bool> uiRunning{true}; // 将 uiRunning 提前定义

    // 消费者线程：专门处理指令，0 轮询
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
            // 🌟 核心逻辑：拦截待机暂停指令
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

    // 生产者 1：UI 触摸（秒回，不卡渲染）
    UI::InteractionManager::setCommandEmitter([&](const System::ControlCommand& cmd) {
        std::lock_guard<std::mutex> lock(cmdMutex);
        cmdQueue.push(cmd);
        cmdCV.notify_one();
    });

    touch.startListening(
        [](int x, int y) { UI::InteractionManager::handleTouch(x, y); },
        [](int deltaY) { UI::InteractionManager::handleScroll(deltaY); }
    );

    std::thread uiThread([&]() {
        System::PlaybackStatus status;

        std::vector<std::string> displayNames;
        for (const auto& pathStr : playlist) {
            displayNames.push_back(fs::path(pathStr).stem().string());
        }
        status.playlist = displayNames;

        // 新增：创建高精度内核定时器 (33.3ms = 30FPS)
        int timer_fd = timerfd_create(CLOCK_MONOTONIC, 0);
        if (timer_fd == -1) {
			std::cerr << "FATAL ERROR: Failed to create hardware timer! RTOS constraints violated." << std::endl;
            exit(EXIT_FAILURE); // 🌟 拿不到硬件中断，直接自爆，绝不用 sleep 苟活！
        }
            struct itimerspec its;
            its.it_value.tv_sec = 0;
            its.it_value.tv_nsec = 33333333; // 首次触发
            its.it_interval.tv_sec = 0;
            its.it_interval.tv_nsec = 33333333; // 循环周期
            timerfd_settime(timer_fd, 0, &its, NULL);

        while (uiRunning) {
            progressMgr.injectTimeData(status);
            status.isPlaying = controller.isPlaying();
            status.volume = static_cast<int32_t>(controller.getVolume() * 100.0f);

            // 增量修复：获取当前正在播放的歌名
            int currentIndex = controller.getCurrentTrackIndex();
            if (currentIndex >= 0 && currentIndex < displayNames.size()) {
                status.songName = displayNames[currentIndex];
            }

            // 开机自动播放拦截器
            // 应对音频底层初始化延迟后导致的“幽灵播放”
            static bool bootAutoPlayIntercepted = false;
            static int bootFrameCounter = 0;
            if (!bootAutoPlayIntercepted) {
                bootFrameCounter++;
                // 只要发现在初始阶段偷偷开始播放了，立刻一枪打断！
                if (status.isPlaying) {
                    System::ControlCommand pauseCmd;
                    pauseCmd.type = System::CommandType::PLAY_PAUSE;
                    controller.processCommand(pauseCmd);
                    status.isPlaying = false; // 同步更新UI状态
                    bootAutoPlayIntercepted = true;
                }
                // 保护机制：如果大约 2 秒内没发生自动播放，或者用户已经主动点屏幕离开了待机界面，解除拦截
                if (bootFrameCounter > 60) {
                    bootAutoPlayIntercepted = true;
                }
            }

            // --- 保持原状：自动连播逻辑 ---
            if (status.isPlaying && status.totalDuration > 0) {
                if (status.currentPosition >= status.totalDuration) {
                    System::ControlCommand nextCmd;
                    nextCmd.type = System::CommandType::NEXT_TRACK;
		    {
            		std::lock_guard<std::mutex> lock(cmdMutex);
            		cmdQueue.push(nextCmd);
            		cmdCV.notify_one(); // 🔔 唤醒后台工人
        	    }
        	    status.isPlaying = false;
    		}
	    }
            // --- 保持原状：频谱能量计算 ---
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

            // 🌟 注入模拟环境数据（后续您在此处替换为真实传感器读取即可）
    //        System::EnvironmentStatus env;
  //          env.temperature = 23.4f; // 示例值
//            env.humidity = 65.0f;    // 示例值
            //env.weatherCode = 2;     // 示例值：2 代表下雨
            //env.cpuLoadPercent = 32; // 示例值
            //env.memUsagePercent = 70;// 示例值

            System::EnvironmentStatus env = envMonitor.getLatestStatus();

            UI::InteractionManager::updateSystemStatus(status, visual);
            UI::InteractionManager::updateEnvStatus(env);
            UI::InteractionManager::renderCurrentPage(&fbUi);

            //使用阻塞 read 等待硬件定时器中断，彻底消灭 Polling (轮询)
            if (timer_fd != -1) {
                uint64_t missed;
                read(timer_fd, &missed, sizeof(missed)); 
            } 

        // 线程退出时清理描述符
        if (timer_fd != -1) close(timer_fd);

    });

// 终端输入。阻塞等待，绝对 0 轮询
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

    // 安全退出序列：Join 必须在 uiRunning 设为 false 之后
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
