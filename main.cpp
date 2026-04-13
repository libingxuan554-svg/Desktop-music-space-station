#include <iostream>           // Include standard input/output stream
#include <string>             // Include standard string class
#include <vector>             // Include dynamic array container
#include <filesystem>         // Include C++17 filesystem library for directory traversal
#include <algorithm>          // Include standard algorithm library for sorting
#include <thread>             // Include low-level multi-threading support
#include <chrono>             // Include high-resolution time library
#include <atomic>             // Include atomic operations for lock-free state machine
#include <cmath>              // Include standard math library
#include <numeric>            // Include numeric algorithm library (e.g., std::accumulate)
#include <sys/timerfd.h>      // Include Linux kernel timer
#include <unistd.h>           // Include POSIX operating system API
#include <mutex>              // Include mutex resource control
#include <condition_variable> // Include condition variable for inter-thread synchronization and wake-up
#include <queue>              // Include queue container for command queuing

#include "SystemInterfaces.hpp" // Include global system communication protocols
#include "WavDecoder.hpp"       // Include audio decoding component
#include "RingBuffer.hpp"       // Include lock-free ring buffer component
#include "MusicController.hpp"  // Include music playback controller
#include "AudioEngine.hpp"      // Include core low-level audio engine
#include "HardwareController.hpp" // Include physical hardware main controller
#include "MediaProgressManager.hpp" // Include media progress and time manager
#include "FramebufferUI.hpp"    // Include low-level VRAM driver wrapper
#include "TouchHandler.hpp"     // Include hardware touchscreen handler
#include "InteractionManager.hpp" // Include high-level interaction state machine
#include "EnvironmentMonitor.hpp" // Include environment perception monitoring module

namespace fs = std::filesystem; // Set namespace alias for filesystem

/**
 * @brief The main entry point of the entire system
 * @note [Architecture / SOLID]:
 * Dependency Injection Center: All low-level modules are instantiated in main and injected via pointers into managers, completely decoupled, perfectly adhering to the Dependency Inversion Principle (DIP).
 */
int main() {

    // 1. Core hardware and audio stream engine initialization
    // ==========================================
    HardwareController hw; // Instantiate hardware main controller
    hw.initialize(); // Mount basic hardware nodes
    hw.initLedStrip(); // Start SPI ambient light strip communication

    WavDecoder decoder; // Instantiate audio unpacker
    RingBuffer buffer(8192); // Initialize high-speed lock-free ring buffer with 8KB capacity
    MusicController controller(&decoder, &buffer); // Instantiate music controller, injecting decoder and buffer
    AudioEngine engine(&controller, &hw); // Instantiate audio engine, injecting controller and hardware abstraction layer

    MediaProgressManager progressMgr(&decoder); // Instantiate media progress manager

    // Start data collection
    EnvironmentMonitor envMonitor; // Instantiate environment sensor monitor
    envMonitor.start(); // Pull up asynchronous sensor reading thread

    // Mount ALSA sound card device
    if (engine.init("plughw:0,0", 44100)) {
        engine.start(); // Pull up highest priority audio interrupt thread
        std::cout << "ALSA Engine Started." << std::endl;
    }

    // 2. Playlist mounting and dictionary construction
    // ==========================================
    std::vector<std::string> playlist; // Pre-allocate media resource storage array
    if (fs::exists("../assets")) { // Check if assets directory exists in physical storage
        for (const auto& entry : fs::directory_iterator("../assets")) { // Iteratively scan all files in the directory
            if (entry.path().extension() == ".wav") { // Filter out illegal non-WAV format files
                playlist.push_back(entry.path().string()); //  Push legal path into list
            }
        }
    }
    std::sort(playlist.begin(), playlist.end()); // Force sort playlist alphabetically
    controller.setPlaylist(playlist); // Inject fully built list into business controller

    // 3. UI rendering pipeline and peripheral initialization
    // ==========================================
    UI::FramebufferUI fbUi("/dev/fb0"); // Mount Linux framebuffer device to gain direct VRAM write access
    UI::TouchHandler touch("/dev/input/event0"); //  Mount Linux core input device to capture hardware interrupts

    // Extreme-speed pre-decompression: Load high-frequency full-screen UI wallpapers into memory dictionary in advance
    fbUi.preloadBmp("STANDBY", "../UI/Renfer/bg_standy_clean_1024x600.bmp");
    fbUi.preloadBmp("MUSIC_LIST", "../UI/Renfer/bg_music_list_clean_1024x600.bmp");
    fbUi.preloadBmp("PLAYER", "../UI/Renfer/bg_music_player_clean_1024x600.bmp");

    /**
     * @brief Core communication bus mechanism
     * @note [Real-Time Constraints]:
     * // [REAL-TIME COMPLIANCE]:
     * // The UI thread is solely responsible for pushing the instructions and then immediately returning;
     * // the underlying background worker suspends using std::condition_variable.
     * // When there is no interaction, it performs 0 polling, fully meeting the strict real-time requirements
     */
    // ==========================================
    std::queue<System::ControlCommand> cmdQueue; // Thread-safe command mailbox queue
    std::mutex cmdMutex; // Mutex protecting concurrent read/write of mailbox
    std::condition_variable cmdCV; // Condition variable for thread communication, used to wake up sleeping worker thread
    std::atomic<bool> uiRunning{true}; // Global lock-free lifecycle atomic flag

    // Consumer thread: Command processing
    // ==========================================
    std::thread cmdWorker([&]() {
        while (true) {
            System::ControlCommand cmd;
            {
                std::unique_lock<std::mutex> lock(cmdMutex); // Apply to access command mailbox
                // Fall asleep until mailbox is not empty or system requests shutdown
                cmdCV.wait(lock, [&]() { return !cmdQueue.empty() || !uiRunning; }); 
                if (!uiRunning && cmdQueue.empty()) break; // Lifecycle ends and commands processed, exit thread
                cmd = cmdQueue.front(); // Take out the earliest arrived letter
                cmdQueue.pop(); // Destroy the original letter
            } // Unlock mutex, return resources immediately

            // Business distribution and special interception logic
            if (cmd.type == System::CommandType::ENTER_STANDBY) { // If it's the enter standby command
                if (controller.isPlaying()) { // Automatically pause playing music when entering standby
                    System::ControlCommand p; p.type = System::CommandType::PLAY_PAUSE;
                    controller.processCommand(p);
                }
            } else {
                controller.processCommand(cmd); // Hand over normal control commands to music controller for processing
            }
        }
    });

    // Bind interaction manager's command emitter to current mailbox system
    UI::InteractionManager::setCommandEmitter([&](const System::ControlCommand& cmd) {
        std::lock_guard<std::mutex> lock(cmdMutex); // Instantly lock mailbox
        cmdQueue.push(cmd); //Deliver letter
        cmdCV.notify_one(); // Ring the bell to wake up background worker thread
    });

    // Pull up asynchronous listening loop for touch hardware and bind UI action callbacks
    touch.startListening(
        [](int x, int y) { UI::InteractionManager::handleTouch(x, y); },
        [](int deltaY) { UI::InteractionManager::handleScroll(deltaY); }
    );

    /**
     * @brief UI rendering main thread
     * @note [Real-Time Constraints]:
     * // [REAL-TIME COMPLIANCE]:
     * // Uses Linux kernel's high-precision timerfd to maintain 30FPS refresh rate.
     * // Discarded all forms of sleep_for, eliminating scheduling jitter under multi-thread contention.
     */
    // ==========================================
    // UI rendering main thread
    std::thread uiThread([&]() {
        System::PlaybackStatus status; // Create local render-exclusive playback status mirror
        std::vector<std::string> displayNames; // Create text container containing only friendly song names
        for (const auto& pathStr : playlist) {
            displayNames.push_back(fs::path(pathStr).stem().string()); // Strip paths and suffixes, keep only pure song names
        }
        status.playlist = displayNames; // Initialize list status buffer

        // Create high-precision kernel timer (33.3ms = 30FPS)
        int timer_fd = timerfd_create(CLOCK_MONOTONIC, 0); // Request monotonic kernel clock independent of system time
        if (timer_fd == -1) { // If hardware descriptors are exhausted
            std::cerr << "FATAL ERROR: Failed to create hardware timer!" << std::endl; // Print fatal error
            exit(EXIT_FAILURE); // Terminate program immediately, absolutely no compromised degradation
        }
        
        struct itimerspec its; // Configure timer trigger beat parameters
        its.it_value.tv_sec = 0;
        its.it_value.tv_nsec = 33333333; // First trigger delay (33.3ms)
        its.it_interval.tv_sec = 0;
        its.it_interval.tv_nsec = 33333333; // Loop cycle (33.3ms)
        timerfd_settime(timer_fd, 0, &its, NULL); // Formally burn parameters into the kernel

        while (uiRunning) { // Main render lifecycle
            progressMgr.injectTimeData(status); // Inject current playback time stream info
            status.isPlaying = controller.isPlaying(); // Sync low-level playback active state
            status.volume = static_cast<int32_t>(controller.getVolume() * 100.0f); // Convert floating volume to readable percentage int

            int currentIndex = controller.getCurrentTrackIndex(); // Get serial number of current active track
            if (currentIndex >= 0 && (size_t)currentIndex < displayNames.size()) {
                status.songName = displayNames[currentIndex]; // Bind legal song name currently playing
            }

            // Auto-continuous playback logic
            if (status.isPlaying && status.totalDuration > 0) { // Confirm audio system is in healthy operational state
                if (status.currentPosition >= status.totalDuration) { // Detect physical playback progress reaching the end
                    System::ControlCommand nextCmd; 
                    nextCmd.type = System::CommandType::NEXT_TRACK; // Build next track command
                    {
                        std::lock_guard<std::mutex> lock(cmdMutex); // Lock mailbox
                        cmdQueue.push(nextCmd); // Push auto track change command into mailbox
                        cmdCV.notify_one(); // Strike the bronze bell, wake up command consumer thread to execute track change
                    }
                }
            }

            // Spectrum energy calculation
            System::AudioVisualData visual; // Create visual energy bearing structure
            if (status.isPlaying) {
                std::vector<float> spectrum = hw.getCurrentSpectrum(); // Grab real-time FFT data stream from hardware base
                if (!spectrum.empty()) {
                    float sum = std::accumulate(spectrum.begin(), spectrum.end(), 0.0f); // Linearly accumulate all band energies
                    // Calculate average, amplify, and clamp to safe normalized interval [0.0, 1.0]
                    visual.overallIntensity = std::clamp((sum / spectrum.size()) * 2.0f, 0.0f, 1.0f); 
                }
            } else {
                visual.overallIntensity = 0.0f; // Stop state directly blocks energy release
            }

            System::EnvironmentStatus env = envMonitor.getLatestStatus(); // Pull physical world data from sensors
            UI::InteractionManager::updateSystemStatus(status, visual); //Inject audio and visual soul into interaction module
            UI::InteractionManager::updateEnvStatus(env); // Inject sensory neuron data into interaction module
            UI::InteractionManager::renderCurrentPage(&fbUi); //  Call core renderer to draw the entire world in VRAM in one breath

            // [REAL-TIME COMPLIANCE]:
            // Pure kernel-level blocking wait for hardware interrupts, completely eliminating polling, zero CPU cycle waste.
            // Blocking wait for hardware timer, completely eliminating Poll
            if (timer_fd != -1) {
                uint64_t missed;
                read(timer_fd, &missed, sizeof(missed)); // Thread completely halts here, yielding control, until hardware clock kicks it awake 33.3ms later
            }
        } 

        if (timer_fd != -1) close(timer_fd); // Release kernel clock resources when thread terminates
    });

    // Terminal input loop (Developer debug channel)
    // ==========================================
    std::string input;
    while (uiRunning && std::cout << ">> " && std::cin >> input) { // Suspend and wait for standard input characters
        System::ControlCommand cmd;
        bool valid = true;
        if (input == "p") cmd.type = System::CommandType::PLAY_PAUSE; //  Map pause
        else if (input == "n") cmd.type = System::CommandType::NEXT_TRACK; // | Map next track
        else if (input == "b") cmd.type = System::CommandType::PREV_TRACK; //  Map previous track
        else if (input == "q") { uiRunning = false; break; } //  Detonate the command that terminates the entire system lifecycle
        else valid = false; //  Filter invalid inputs

        if (valid) { // Safe terminal control info delivery mechanism
            std::lock_guard<std::mutex> lock(cmdMutex);
            cmdQueue.push(cmd);
            cmdCV.notify_one();
        }
    }

    /**
     * @brief  Safe exit sequence
     * @note [Architecture / SOLID]:
     *  Strictly destroy threads and hardware handles in reverse hierarchical order to prevent segfaults and zombie processes.
     */
    // ==========================================
    // Safe exit sequence
    uiRunning = false; // Lower the global life flag
    cmdCV.notify_all(); // Wake up all possibly sleeping suspended threads, let them see the flag lowered and terminate themselves
    if (cmdWorker.joinable()) cmdWorker.join(); //  Block and recycle messenger thread
    if (uiThread.joinable()) uiThread.join(); //  Block and recycle painter thread

    touch.stopListening(); //  Disconnect physical touch input interrupts
    engine.stop(); //  Forcibly halt the massive ALSA low-level sound card engine
    hw.shutdown(); //  Power down and reset peripheral physical pins
    envMonitor.stop(); //  End the polling life of the environment sensor
    return 0; //  Space station perfectly extinguished, system-level safe exit
}
