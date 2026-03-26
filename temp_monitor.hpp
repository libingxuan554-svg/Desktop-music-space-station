#ifndef DS18B20_HPP
#define DS18B20_HPP

#include <string>

// Get the file path of the DS18B20 device
std::string get_sensor_path();

// Read and parse the temperature
float read_temperature(const std::string& sensor_path);

#endif // DS18B20_HPP
