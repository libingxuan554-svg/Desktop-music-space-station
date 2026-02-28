# Desktop-music-space-station
This project is a high-performance, C++-based embedded music player designed for the Raspberry Pi. It provides a touch-controlled interface  while strictly adhering to Real-Time System design principles.
##  Team 15
* **BingXuan Li** (Team Leader) 
* **Yang Zhang** (GUI Programmer)
* **ZiKai Ma** (Hardware Programmer)
* **JiaNan Liu** (Code Programmer)
* **MingCong Xu** (Code Programmer)

##  Project Overview
This project transforms sound into physical light and shadow flows in strictly synchronized real-time. It features an Adaptive Dual-Mode System that switches between a silent Ambient Mode and a high-performance Rhythm Mode using physical environment sensing.

### Real-Time Strategy & Architecture
* **Event-Driven & Callbacks**: Strictly adheres to event-driven programming. Hardware events (like ALSA PCM data availability or PIR sensor triggers) unblock I/O operations and wake up worker threads via `poll()`, avoiding CPU-intensive polling loops.
* **Multithreading**: Divided into 3 concurrent threads (UI, Signal Processing via FFT, Audio I/O) to guarantee latency under 20ms.
* **Direct Hardware Access**: Utilizes SPI interface via DMA for WS2812B LED control to ensure minimal latency.

##  Hardware Requirements
* Raspberry Pi 5
* Waveshare 7-inch Touchscreen
* BTF-LIGHTING WS2812B LED Strip
* AM312 Mini PIR Motion Sensor (Connected via GPIO for interrupt generation)
* Logic Level Converter (3.3V to 5V)

##  Software Dependencies
To build and run this project, you need the following libraries installed on your Raspberry Pi:
```bash
# Update system packages
sudo apt-get update

# Install ALSA for audio I/O
sudo apt-get install libasound2-dev

# Install Qt framework for GUI
sudo apt-get install qtbase5-dev qt5-qmake

# Install FFTW (or your chosen FFT library)
sudo apt-get install libfftw3-dev

# Clone the repository
git clone [https://github.com/your-repo-link/space-station.git](https://github.com/your-repo-link/space-station.git)
cd space-station

# Create build directory
mkdir build && cd build

# Configure and compile
cmake ..
make -j4

# Run the executable
sudo ./SpaceStation
