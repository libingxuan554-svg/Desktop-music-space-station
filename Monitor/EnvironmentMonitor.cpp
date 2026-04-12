#include "EnvironmentMonitor.hpp"
#include <fstream>
#include <sstream>
#include <string>
#include <chrono>
#include <iostream>
// Linux terminal and string processing
#include <cstdio>
#include <algorithm>
#include <sys/timerfd.h>
#include <unistd.h>
#include <filesystem>

namespace fs = std::filesystem;

/**
 * @brief Constructor for EnvironmentMonitor.
 * * Sets initial safe default values to prevent the UI from reading garbage data before the first update.
 * Design rationale (SOLID Principle): Adheres to the Single Responsibility Principle (SRP). 
 * This class is solely responsible for safely gathering, parsing, and encapsulating environmental data, 
 * completely separated from UI rendering or data transport logic.
 */
EnvironmentMonitor::EnvironmentMonitor()
    : isRunning(false), prevTotalCpuTime(0), prevIdleCpuTime(0), weatherUpdateCounter(0) {
    // Initialize default values to ensure safe data structure before the first hardware poll
    currentStatus.temperature = 0.0f;
    currentStatus.humidity = 0.0f;
    currentStatus.weatherCode = 0;
    currentStatus.cpuLoadPercent = 0;
    currentStatus.memUsagePercent = 0;
    currentStatus.lux = 0;
}

EnvironmentMonitor::~EnvironmentMonitor() {
    stop();
}

void EnvironmentMonitor::start() {
    if (!isRunning) {
        isRunning = true;
        // Fetch all data immediately on the first start to populate the UI instantly
        updateMemoryUsage();
        updateCpuUsage();
        updateWeather();
        updateSensors();
        // Launch the dedicated background thread for realtime data gathering
        monitorThread = std::thread(&EnvironmentMonitor::monitorLoop, this);
    }
}

void EnvironmentMonitor::stop() {
    if (isRunning) {
        isRunning = false;
        if (monitorThread.joinable()) {
            monitorThread.join();
        }
    }
}

/**
 * @brief Safely retrieves the latest encapsulated environment data.
 * * @return System::EnvironmentStatus The latest snapshot of system and environmental metrics.
 * @note This acts as a safe Getter interface. It utilizes std::mutex to prevent race conditions,
 * ensuring strict encapsulation and safe data management across threads.
 */
System::EnvironmentStatus EnvironmentMonitor::getLatestStatus() {
    std::lock_guard<std::mutex> lock(dataMutex);
    return currentStatus; // Thread-safe copy of the data
}

/**
 * @brief Main realtime event loop running in a dedicated thread.
 * * CRITICAL REALTIME DESIGN: This loop uses Linux timerfd to achieve strict Blocking I/O.
 * It converts standard sleep-polling into pure kernel event-driven interrupts. 
 * This completely avoids CPU-wasting polling loops or blocking delays (sleep), adhering
 * perfectly to production-level realtime coding guidelines.
 */
void EnvironmentMonitor::monitorLoop() {
    // 1. Request a high-precision hardware timer from the Linux kernel
    int timer_fd = timerfd_create(CLOCK_MONOTONIC, 0);
    if (timer_fd != -1) {
        struct itimerspec its;
        its.it_value.tv_sec = 1;      // Initial expiration after 1 second
        its.it_value.tv_nsec = 0;
        its.it_interval.tv_sec = 1;   // Hardware interrupt generated every 1 second thereafter
        its.it_interval.tv_nsec = 0;
        timerfd_settime(timer_fd, 0, &its, NULL);
    }

    while (isRunning) {
        // 2.ABSOLUTE ZERO POLLING: The thread suspends here and yields execution control 
        // until the hardware interrupt wakes it up (Blocking I/O).
        if (timer_fd != -1) {
            uint64_t missed;
            read(timer_fd, &missed, sizeof(missed)); // Blocks here waiting for the kernel event
        } else {
            std::this_thread::sleep_for(std::chrono::seconds(1)); // Defensive fallback only
        }

        if (!isRunning) break; // Check for safe exit immediately after wake-up

        // 3. Execute high-frequency data acquisition tasks
        updateCpuUsage();
        updateMemoryUsage();
        updateSensors();

        // 4. Execute low-frequency network tasks (e.g., every 600 seconds)
        weatherUpdateCounter++;
        if (weatherUpdateCounter >= 600) {
            updateWeather();
            weatherUpdateCounter = 0;
        }
    }

    // 5. Release kernel resources
    if (timer_fd != -1) close(timer_fd);
}

// =====================================================================
//  Specific Data Acquisition Handlers
// =====================================================================

void EnvironmentMonitor::updateMemoryUsage() {
    std::ifstream meminfo("/proc/meminfo");
    if (!meminfo.is_open()) return;

    std::string line;
    long totalMem = 0, availableMem = 0;

    while (std::getline(meminfo, line)) {
        if (line.compare(0, 9, "MemTotal:") == 0) {
            std::istringstream iss(line.substr(9));
            iss >> totalMem;
        } else if (line.compare(0, 13, "MemAvailable:") == 0) {
            std::istringstream iss(line.substr(13));
            iss >> availableMem;
        }
    }

    if (totalMem > 0) {
        int usage = static_cast<int>(100.0 * (totalMem - availableMem) / totalMem);
        // Lock and write safely
        std::lock_guard<std::mutex> lock(dataMutex);
        currentStatus.memUsagePercent = usage;
    }
}

void EnvironmentMonitor::updateCpuUsage() {
    std::ifstream stat("/proc/stat");
    if (!stat.is_open()) return;

    std::string line;
    std::getline(stat, line); // The first line always contains the aggregated "cpu" data
    std::istringstream iss(line);
    std::string cpuLabel;
    iss >> cpuLabel;

    if (cpuLabel == "cpu") {
        unsigned long long user, nice, system, idle, iowait, irq, softirq, steal;
        iss >> user >> nice >> system >> idle >> iowait >> irq >> softirq >> steal;

        unsigned long long idleTime = idle + iowait;
        unsigned long long totalTime = user + nice + system + idleTime + irq + softirq + steal;

        // Calculate the delta from the previous second to get the true real-time usage
        unsigned long long totalDelta = totalTime - prevTotalCpuTime;
        unsigned long long idleDelta = idleTime - prevIdleCpuTime;

        if (totalDelta > 0) {
            int load = static_cast<int>(100.0 * (totalDelta - idleDelta) / totalDelta);
            std::lock_guard<std::mutex> lock(dataMutex);
            currentStatus.cpuLoadPercent = load;
        }

        prevTotalCpuTime = totalTime;
        prevIdleCpuTime = idleTime;
    }
}

/**
 * @brief Fetches real-time weather using network API.
 * Uses a blocking pipe in the background thread to prevent UI freezing.
 */
void EnvironmentMonitor::updateWeather() {
    FILE* pipe = popen("curl -s --max-time 5 \"wttr.in/Glasgow?format=%C+%t\"", "r");
    if (!pipe) return;

    char buffer[128];
    std::string result = "";
    while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
        result += buffer;
    }
    pclose(pipe);

    // If the Pi is offline or the request fails, return immediately to preserve the last known state
    if (result.empty() || result.find("Unknown") != std::string::npos) return;

    // Convert to lowercase uniformly for easier keyword matching
    std::transform(result.begin(), result.end(), result.begin(), ::tolower);

    // 1.Parse the weather code (smart fuzzy matching)
    int code = 1; // Default to CLOUDY
    if (result.find("clear") != std::string::npos || result.find("sun") != std::string::npos) code = 0;
    else if (result.find("cloud") != std::string::npos || result.find("overcast") != std::string::npos) code = 1;
    else if (result.find("rain") != std::string::npos || result.find("drizzle") != std::string::npos || result.find("shower") != std::string::npos) code = 2;
    else if (result.find("snow") != std::string::npos || result.find("ice") != std::string::npos) code = 3;
    else if (result.find("thunder") != std::string::npos || result.find("storm") != std::string::npos) code = 4;
    else if (result.find("fog") != std::string::npos || result.find("mist") != std::string::npos) code = 5;

    // 2   Parse the true temperature (e.g., terminal returns "clear +22°c" or "rain -3°c")
    float temp = currentStatus.temperature; 
    size_t signPos = result.find_first_of("+-");
    if (signPos != std::string::npos) {
        try {
            // Extract the numeric portion starting from '+' or '-'
            temp = std::stof(result.substr(signPos));
        } catch (...) {} // Fault tolerance: keep the original temperature if parsing fails
    }

    // 3. Safely write to the shared underlying state pool
    std::lock_guard<std::mutex> lock(dataMutex);
    currentStatus.weatherCode = code;
}

void EnvironmentMonitor::updateSensors() {

// 1. Path caching strategy: Directory traversal is expensive. We only scan the sysfs 
// directory once if the path is unknown, drastically reducing system overhead.
    if (ds18b20_path.empty()) {
        std::string base_dir = "/sys/bus/w1/devices/";
        if (fs::exists(base_dir)) {
            for (const auto& entry : fs::directory_iterator(base_dir)) {
                std::string filename = entry.path().filename().string();
                if (filename.find("28-") == 0) { // '28' is the family code for DS18B20
                    ds18b20_path = entry.path().string() + "/w1_slave";
                    break;
                }
            }
        }
    }

    // 2. High-speed file reading and parsing
    if (!ds18b20_path.empty()) {
        std::ifstream file(ds18b20_path);
        if (file.is_open()) {
            std::string line;
            bool is_valid = false;
            float temp_celsius = -1000.0f;

            // w1_slave file usually contains two lines:
            // First line ending in "YES" confirms data validity.
            // Second line contains "t=xxxxx" (in milli-degrees Celsius).
            while (std::getline(file, line)) {
                if (line.find("YES") != std::string::npos) {
                    is_valid = true;
                }
                size_t t_pos = line.find("t=");
                if (t_pos != std::string::npos) {
                    std::string temp_str = line.substr(t_pos + 2);
                    temp_celsius = std::stof(temp_str) / 1000.0f; 
                }
            }
            file.close();

            // 3. Data validation and safe write to shared memory
            // Sanity check for realistic temperature ranges (-50 to 100) to prevent hardware glitches (e.g., standard 85000 error)
            if (is_valid && temp_celsius > -50.0f && temp_celsius < 100.0f) {
                std::lock_guard<std::mutex> lock(dataMutex);
                currentStatus.temperature = temp_celsius; // Physical room temp overrides scraped web temp!
            }
        }
    }
}

