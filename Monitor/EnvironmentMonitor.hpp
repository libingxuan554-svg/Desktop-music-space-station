#ifndef ENVIRONMENT_MONITOR_HPP
#define ENVIRONMENT_MONITOR_HPP

#include "SystemInterfaces.hpp"
#include <thread>
#include <atomic>
#include <mutex>

class EnvironmentMonitor {
public:
    EnvironmentMonitor();
    ~EnvironmentMonitor();

    // 启动后台采集线程
    void start();
    // 停止采集线程
    void stop();

    // 供外部 (如 UI 线程) 随时安全获取最新整合好数据的唯一接口
    System::EnvironmentStatus getLatestStatus();

private:
    void monitorLoop();

    // 各个具体的数据采集工位 (私有函数，对外隐藏)
    void updateCpuUsage();
    void updateMemoryUsage();
    void updateWeather();
    void updateSensors();

    // 线程与同步控制
    std::thread monitorThread;
    std::atomic<bool> isRunning;
    std::mutex dataMutex;

    // 核心数据聚合体
    System::EnvironmentStatus currentStatus;

    // 用于计算 CPU 占用率的历史数据状态
    unsigned long long prevTotalCpuTime;
    unsigned long long prevIdleCpuTime;

    // 用于控制低频刷新的计时器 (比如天气没必要每秒刷，10分钟刷一次即可)
    int weatherUpdateCounter;

    // 缓存 DS18B20 传感器的文件路径
    std::string ds18b20_path;

};

#endif // ENVIRONMENT_MONITOR_HPP
