#ifndef ENVIRONMENT_MONITOR_HPP
#define ENVIRONMENT_MONITOR_HPP

#include "SystemInterfaces.hpp"
#include <thread>
#include <atomic>
#include <mutex>

/**
 * @brief Manages the real-time acquisition of hardware and software environmental metrics.
 * * Design rationale (SOLID Principle - SRP): 
 * Adheres to the Single Responsibility Principle. This class is strictly responsible for 
 * monitoring and aggregating data (CPU, memory, weather, and 1-Wire sensors) via blocking I/O 
 * in a dedicated background thread. It perfectly encapsulates the complexity of data gathering 
 * and provides a safe, thread-locked interface for the UI to consume without exposing internal states.
 */
class EnvironmentMonitor {
public:
    /**
     * @brief Constructor for EnvironmentMonitor.
     */
    EnvironmentMonitor();

    /**
     * @brief Destructor for EnvironmentMonitor. Ensures safe thread cleanup.
     */
    ~EnvironmentMonitor();

    /**
     * @brief Starts the background data acquisition thread.
     */
    void start();

    /**
     * @brief Safely stops the background data acquisition thread.
     */
    void stop();

    /**
     * @brief The sole safe interface for external threads (e.g., UI) to fetch the latest aggregated data.
     * @return System::EnvironmentStatus A thread-safe copy of the latest system and environmental metrics.
     */
    System::EnvironmentStatus getLatestStatus();

private:
     /**
     * @brief Main realtime event loop running in the background thread.
     */
    void monitorLoop();

    // Specific data acquisition handlers (Private to enforce encapsulation)
    void updateCpuUsage();
    void updateMemoryUsage();
    void updateWeather();
    void updateSensors();

    // Threading and synchronization controls
    std::thread monitorThread;
    std::atomic<bool> isRunning;
    std::mutex dataMutex;

    // Core aggregated data structure
    System::EnvironmentStatus currentStatus;

    // Historical states for calculating real-time CPU load
    unsigned long long prevTotalCpuTime;
    unsigned long long prevIdleCpuTime;

    // Timer for low-frequency updates (e.g., weather updates every 10 minutes instead of every second)
    int weatherUpdateCounter;

    // Cached file path for the DS18B20 1-Wire sensor to avoid expensive directory traversals
    std::string ds18b20_path;

};

#endif // ENVIRONMENT_MONITOR_HPP
