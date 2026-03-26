#include "ds18b20.hpp"
#include <iostream>
#include <thread>
#include <chrono>

int main() {
    std::cout << "Initializing DS18B20 temperature sensor..." << std::endl;

    std::string sensor_path = get_sensor_path();

    if (sensor_path.empty()) {
        std::cerr << "Error: DS18B20 sensor not found!" << std::endl;
        std::cerr << "Please ensure the sensor is connected and the 1-Wire interface is enabled on the Raspberry Pi." << std::endl;
        return 1;
    }

    std::cout << "Sensor found, device path: " << sensor_path << std::endl;
    std::cout << "Starting real-time temperature monitoring (Press Ctrl+C to exit)..." << std::endl;
    std::cout << "----------------------------------------" << std::endl;

    // Real-time monitoring loop
    while (true) {
        float temp = read_temperature(sensor_path);
        
        if (temp > -999.0f) {
            std::cout << "\rCurrent temperature: " << temp << " °C" << std::flush;
        } else {
            std::cout << "\rFailed to read temperature, retrying..." << std::flush;
        }

        // Delay for 1 second
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    return 0;
}
