#include "ds18b20.hpp"
#include <fstream>
#include <iostream>
#include <filesystem>

namespace fs = std::filesystem;

std::string get_sensor_path() {
    std::string base_dir = "/sys/bus/w1/devices/";
    
    // Check if the base directory exists (whether the 1-Wire driver is loaded)
    if (!fs::exists(base_dir)) {
        return "";
    }

    // Iterate through the directory to find a folder starting with "28-" (DS18B20 family code is 28)
    for (const auto& entry : fs::directory_iterator(base_dir)) {
        std::string filename = entry.path().filename().string();
        if (filename.find("28-") == 0) {
            return entry.path().string() + "/w1_slave";
        }
    }
    return "";
}

float read_temperature(const std::string& sensor_path) {
    std::ifstream file(sensor_path);
    if (!file.is_open()) {
        std::cerr << "Failed to read the sensor file!" << std::endl;
        return -1000.0f; // Return an error value
    }

    std::string line;
    bool is_valid = false;
    float temperature = -1000.0f;

    // The w1_slave file usually has two lines
    // The first line ending with "YES" indicates a successful read
    // The second line contains "t=temperature" (in millidegrees Celsius)
    while (std::getline(file, line)) {
        if (line.find("YES") != std::string::npos) {
            is_valid = true;
        }
        
        size_t t_pos = line.find("t=");
        if (t_pos != std::string::npos) {
            std::string temp_str = line.substr(t_pos + 2);
            temperature = std::stof(temp_str) / 1000.0f; // Convert to degrees Celsius
        }
    }

    file.close();

    if (is_valid) {
        return temperature;
    } else {
        return -1000.0f; // Data validation failed
    }
}
