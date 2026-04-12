#include "EnvironmentMonitor.hpp"
#include <fstream>
#include <sstream>
#include <string>
#include <chrono>
#include <iostream>
// 调用 Linux 终端和字符串处理
#include <cstdio>
#include <algorithm>
#include <sys/timerfd.h>
#include <unistd.h>
#include <filesystem>

namespace fs = std::filesystem;

EnvironmentMonitor::EnvironmentMonitor()
    : isRunning(false), prevTotalCpuTime(0), prevIdleCpuTime(0), weatherUpdateCounter(0) {
    // 初始化默认值，防止一开始没数据时 UI 拿到底层乱码
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
        // 首次启动时立刻强刷一次所有数据
        updateMemoryUsage();
        updateCpuUsage();
        updateWeather();
        updateSensors();
        // 开启后台采集循环
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

System::EnvironmentStatus EnvironmentMonitor::getLatestStatus() {
    std::lock_guard<std::mutex> lock(dataMutex);
    return currentStatus; // 安全拷贝并返回最新数据
}

// 使用 timerfd 将睡眠轮询转化为“纯内核事件驱动”
void EnvironmentMonitor::monitorLoop() {
    // 1. 向 Linux 内核申请一个高精度硬件定时器
    int timer_fd = timerfd_create(CLOCK_MONOTONIC, 0);
    if (timer_fd != -1) {
        struct itimerspec its;
        its.it_value.tv_sec = 1;      // 首次触发等待 1 秒
        its.it_value.tv_nsec = 0;
        its.it_interval.tv_sec = 1;   // 之后每隔 1 秒产生一次硬件中断
        its.it_interval.tv_nsec = 0;
        timerfd_settime(timer_fd, 0, &its, NULL);
    }

    while (isRunning) {
        // 2.绝对零轮询：线程在此处挂起交出控制权，直到硬件中断唤醒它
        if (timer_fd != -1) {
            uint64_t missed;
            read(timer_fd, &missed, sizeof(missed)); // 阻塞等待内核事件
        } else {
            std::this_thread::sleep_for(std::chrono::seconds(1)); // 仅作防御性保底
        }

        if (!isRunning) break; // 唤醒后立刻检查是否需要安全退出

        // 3. 执行高频采集任务
        updateCpuUsage();
        updateMemoryUsage();
        updateSensors();

        // 4. 执行低频网络任务 (每 600 秒)
        weatherUpdateCounter++;
        if (weatherUpdateCounter >= 600) {
            updateWeather();
            weatherUpdateCounter = 0;
        }
    }

    // 5. 释放内核资源
    if (timer_fd != -1) close(timer_fd);
}

// =====================================================================
//  各个具体采集岗位
// =====================================================================

//  读取真实的 Linux 内存信息 (/proc/meminfo)
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
        // 加锁写入
        std::lock_guard<std::mutex> lock(dataMutex);
        currentStatus.memUsagePercent = usage;
    }
}

// 🌟 读取真实的 Linux CPU 负载 (/proc/stat)
void EnvironmentMonitor::updateCpuUsage() {
    std::ifstream stat("/proc/stat");
    if (!stat.is_open()) return;

    std::string line;
    std::getline(stat, line); // 第一行总是总的 "cpu" 数据
    std::istringstream iss(line);
    std::string cpuLabel;
    iss >> cpuLabel;

    if (cpuLabel == "cpu") {
        unsigned long long user, nice, system, idle, iowait, irq, softirq, steal;
        iss >> user >> nice >> system >> idle >> iowait >> irq >> softirq >> steal;

        unsigned long long idleTime = idle + iowait;
        unsigned long long totalTime = user + nice + system + idleTime + irq + softirq + steal;

        // 计算与上一秒的差值来得出真实的实时占用率
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

// 网络天气获取
void EnvironmentMonitor::updateWeather() {
    FILE* pipe = popen("curl -s --max-time 5 \"wttr.in/Glasgow?format=%C+%t\"", "r");
    if (!pipe) return;

    char buffer[128];
    std::string result = "";
    while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
        result += buffer;
    }
    pclose(pipe);

    // 如果树莓派断网或请求失败，直接返回，保持屏幕上的上一次状态
    if (result.empty() || result.find("Unknown") != std::string::npos) return;

    // 统一转换为小写，方便接下来的关键字匹配
    std::transform(result.begin(), result.end(), result.begin(), ::tolower);

    // 1.解析天气代码 智能模糊匹配)
    int code = 1; // 默认多云 (CLOUDY)
    if (result.find("clear") != std::string::npos || result.find("sun") != std::string::npos) code = 0;
    else if (result.find("cloud") != std::string::npos || result.find("overcast") != std::string::npos) code = 1;
    else if (result.find("rain") != std::string::npos || result.find("drizzle") != std::string::npos || result.find("shower") != std::string::npos) code = 2;
    else if (result.find("snow") != std::string::npos || result.find("ice") != std::string::npos) code = 3;
    else if (result.find("thunder") != std::string::npos || result.find("storm") != std::string::npos) code = 4;
    else if (result.find("fog") != std::string::npos || result.find("mist") != std::string::npos) code = 5;

    // 2   解析真实温度 (例如终端返回的是 "clear +22°c" 或 "rain -3°c")
    float temp = currentStatus.temperature; 
    size_t signPos = result.find_first_of("+-");
    if (signPos != std::string::npos) {
        try {
            // 从 '+' 或 '-' 开始截取数字部分
            temp = std::stof(result.substr(signPos));
        } catch (...) {} // 容错保护：转换失败则保持原温度
    }

    // 3. 🔒 安全写入底层状态共享池
    std::lock_guard<std::mutex> lock(dataMutex);
    currentStatus.weatherCode = code;
}

// DS18B20 硬件传感器获取
void EnvironmentMonitor::updateSensors() {

// 1. 路径缓存寻址：只在第一次找不到路径时遍历文件夹，极大节省系统开销
    if (ds18b20_path.empty()) {
        std::string base_dir = "/sys/bus/w1/devices/";
        if (fs::exists(base_dir)) {
            for (const auto& entry : fs::directory_iterator(base_dir)) {
                std::string filename = entry.path().filename().string();
                if (filename.find("28-") == 0) { // DS18B20 的家族代码是 28
                    ds18b20_path = entry.path().string() + "/w1_slave";
                    break;
                }
            }
        }
    }

    // 2. 极速文件读取与解析
    if (!ds18b20_path.empty()) {
        std::ifstream file(ds18b20_path);
        if (file.is_open()) {
            std::string line;
            bool is_valid = false;
            float temp_celsius = -1000.0f;

            // w1_slave 文件通常有两行：
            // 第一行末尾 "YES" 代表数据有效
            // 第二行包含 "t=xxxxx" (千分之一摄氏度)
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

            // 3. 数据校验并安全写入共享内存
            // 加入一个合理的温度范围校验 (-50 到 100)，防止传感器抽风输出 85000 这种错误值
            if (is_valid && temp_celsius > -50.0f && temp_celsius < 100.0f) {
                std::lock_guard<std::mutex> lock(dataMutex);
                currentStatus.temperature = temp_celsius; // 真实物理室温覆盖掉网络爬取的温度！
            }
        }
    }
}

