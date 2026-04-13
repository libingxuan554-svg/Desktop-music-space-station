# Desktop Music Space Station 

> **An Industrial-Grade Real-Time Embedded Audio Subsystem for Raspberry Pi.**
> Engineered for **ENG 5220: Real Time Embedded Programming**, University of Glasgow.
> 
> Desktop Music Space Station is a high-performance, C++17-powered embedded music terminal. It integrates a deterministic lock-free audio decoding engine, an ultra-low latency Framebuffer GUI, and hardware-level environment monitoring. Designed with a strict Zero-Polling architecture, the system guarantees exact 30FPS UI rendering and glitch-free audio playback under rigorous real-time computing constraints.

##  1. Core Architectural Highlights (Real-Time Compliance)

* **Zero Polling & Event-Driven Execution:**
  * The entire system operates with **the highest 15% CPU utilization**.
  * UI rendering is strictly driven by the Linux Kernel `timerfd`, guaranteeing exact 30Hz hardware-interrupted rendering cycles.
  * The touchscreen subsystem relies on pure blocking `read()` on the `evdev` node, completely suspending thread execution when no physical interrupts occur.
* **Lock-Free Concurrency & Synchronization:**
  * The audio decoding thread and the ALSA playback thread communicate through a **Lock-Free `RingBuffer`** utilizing `std::atomic<size_t>` with strict memory ordering (`std::memory_order_acquire`/`release`), eliminating audio stutter caused by mutex priority inversion.
  * Inter-thread command dispatching (UI to Audio) and Hardware LED updates utilize `std::condition_variable` to wake up dormant worker threads instantly.
* **SOLID Principles & Clean Architecture:**
  * **Dependency Inversion Principle (DIP):** The `AudioEngine` is completely decoupled from the file format, depending solely on the abstract `AudioSource` interface.
  * **Single Responsibility Principle (SRP):** Hardware abstraction (`FramebufferUI`), rendering algorithms (`UIRenderer`), and state management (`InteractionManager`) are strictly isolated.

##  2. Hardware Dependencies & Setup

To run this project, the following hardware setup and specific Linux device nodes are strictly required:

* **Audio Output:** ALSA-compatible device (e.g., Bluetooth speaker or headphone jack). Hardcoded to route via `plughw:0,0`.
* **Display Output:** Linux Framebuffer device mapped at `/dev/fb0` (Resolution: 1024x600, 16-bit or 32-bit color depth).
* **Touch Input:** Capacitive touch screen configured at `/dev/input/event0` (Requires `EV_ABS` absolute coordinate support).
* **LED Strip:** WS2812/SK6812 LED strip connected via Hardware SPI at `/dev/spidev0.0` (8MHz clock speed).
* **Thermal Sensor:** DS18B20 (1-Wire interface) mapped dynamically via `/sys/bus/w1/devices/28-*`.

##  3. Software Dependencies

This project strictly adheres to **AUTOSAR C++14/17 guidelines** for resource management. Memory is governed by RAII principles and STL containers (`std::vector`, `std::string`); bare `new`/`delete` calls are entirely prohibited to prevent memory fragmentation and leaks.

* **C++ Standard:** C++17
* **Build System:** CMake (Minimum version 3.10)
* **System Libraries:**
  * `libasound2-dev` (Advanced Linux Sound Architecture API)
  * `pthread` (POSIX Threads)
  * `stdc++fs` (C++17 Filesystem)
  * `libfftw3-dev` / `curl` (For advanced monitoring and weather fetching)
  
##  4. Directory Structure & Modularity

The project is modularized to adhere to the Single Responsibility Principle:

/UI: Contains the strictly decoupled Framebuffer memory-mapping driver, non-blocking touch event parser, and stateless .UIRenderer

/MPM (Media Progress Manager): Handles WAV binary decoding, Lock-Free RingBuffers, Goertzel spectrum extraction, and the blocking ALSA Audio Engine.

/Monitor: Background daemons utilizing  to gather real-time system load, 1-Wire sensor temperatures, and network weather.timerfd

/assets: Houses  music binaries..wav

/UI/Renfer: Houses pre-rendered  UI backgrounds loaded into RAM at startup to achieve Zero Disk I/O during runtime..bmp

## 5. Engineering Workflow & Version Control
Test-Driven Development (TDD): During the actual development process, a significant amount of work was focused on the Raspberry Pi Linux system, where code was written, iterated, and optimized through continuous interaction with the hardware. Business logic, coordinate clipping algorithms, and state machines were verified independently through the physical hardware.

Version Control: Continuous integration was maintained throughout the development cycle. The Git commit history reflects a steady, iterative refinement process—from eliminating initial polling loops to implementing the final event-driven lock-free architecture—demonstrating a professional software engineering lifecycle rather than a single bulk upload.

## 6. Team 15 & Contributions
This project was collaboratively engineered by Team 15:

BingXuan Li — Team Leader / System Architect

Yang Zhang — GUI Programmer / Data Systems Developer

MingCong Xu — Audio Systems Engineer / MPM Core Developer

JiaNan Liu — Hardware Programmer / BSP & Driver Integration

ZiKai Ma — Hardware Programmer / BSP & Driver Integration

## 7. Build and Run Instructions

A `setup.sh` script is provided to configure the environment automatically. Execute the following commands in your Raspberry Pi terminal:

```bash
#1. Clone the repository (Replace with your actual GitHub repository URL)
git clone [https://github.com/libingxuan554-svg/Desktop-music-space-station.git]
cd Desktop-music-space-station

#2. Execute the environment configuration script (Installs dependencies and sets group permissions)
chmod +x setup.sh
./setup.sh

#3. setup coding environment
./setup.sh
sudo systemctl stop lightdm

#4. coding & run
cd ~/Desktop-music-space-station/build && rm -rf * && cmake .. && make MusicStation -j4
export XDG_RUNTIME_DIR=/run/user/1000 && sudo -E ALSA_LOG_LEVEL=quiet PULSE_SERVER=unix:/run/user/1000/pulse/native ./MusicStation



Glasgow, Spring 2026
