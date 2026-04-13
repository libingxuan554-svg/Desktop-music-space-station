# Desktop Music Space Station 

> **An Industrial-Grade Real-Time Embedded Audio Subsystem for Raspberry Pi.**
> Engineered for **ENG 5220: Real Time Embedded Programming**, University of Glasgow.
> 
> This C++17-powered terminal is designed to operate under rigorous real-time computing constraints, guaranteeing glitch-free audio playback and a deterministic 30FPS UI rendering cycle. By integrating a lock-free audio decoding engine with a strict Zero-Polling architecture, the system eliminates common embedded bottlenecks like audio stutter and high CPU idling, ensuring that hardware resources are utilized with maximum efficiency and precision.
> 
> <img width="450" height="350" alt="image" src="https://github.com/user-attachments/assets/9db4cddf-4f23-42e4-8c53-6b281f544b31" />


## 1.Design Requirements & Core Architectural Highlights (Real-Time Compliance)

At its core, the system is built upon the principle of Deterministic Real-Time Performance. Unlike traditional desktop applications that rely on heavy OS scheduling, this project minimizes jitter through a Zero-Polling & Event-Driven architecture. The design prioritizes:

* **Zero Polling & Event-Driven Execution:**
  * The entire system operates with **the highest 25% CPU utilization**.
  * UI rendering is strictly driven by the Linux Kernel `timerfd`, guaranteeing exact 30Hz hardware-interrupted rendering cycles.
  * The touchscreen subsystem relies on pure blocking `read()` on the `evdev` node, completely suspending thread execution when no physical interrupts occur.
* **Lock-Free Concurrency & Synchronization:**
  * The audio decoding thread and the ALSA playback thread communicate through a **Lock-Free `RingBuffer`** utilizing `std::atomic<size_t>` with strict memory ordering (`std::memory_order_acquire`/`release`), eliminating audio stutter caused by mutex priority inversion.
  * Inter-thread command dispatching (UI to Audio) and Hardware LED updates utilize `std::condition_variable` to wake up dormant worker threads instantly.
* **SOLID Principles & Clean Architecture:**
  * **Dependency Inversion Principle (DIP):** The `AudioEngine` is completely decoupled from the file format, depending solely on the abstract `AudioSource` interface.
  * **Single Responsibility Principle (SRP):** Hardware abstraction (`FramebufferUI`), rendering algorithms (`UIRenderer`), and state management (`InteractionManager`) are strictly isolated.

##  2. Technical Specifications

The design translates high-level requirements into quantifiable technical boundaries, ensuring the system remains stable and responsive under peak loads. This implementation adheres to **AUTOSAR C++14/17 guidelines**, strictly prohibiting bare `new/delete` calls to eliminate memory fragmentation and leaks. 
| Dimension | Implementation | Performance Metric | 
| :--- | :--- | :--- | 
| **UI Rendering** | Linux Framebuffer + `timerfd` | Constant 30 FPS | 
| **Audio Sync** | Lock-Free RingBuffer (`std::atomic`) | Zero Stutter / Glitch-free | 
| **Input Handling** | Blocking `read()` on `evdev` | 25% Idle CPU Usage | 
| **Memory Mgmt** | Strict RAII & STL Containers | Zero Fragmentation | 
| **Thermal Monitoring** | DS18B20 (1-Wire Interface) | Real-time accuracy |

##  3. Hardware Dependencies & Setup

To run this project, the following hardware setup and specific Linux device nodes are strictly required:

* **Audio Output:** ALSA-compatible device (e.g., Bluetooth speaker or headphone jack). Hardcoded to route via `plughw:0,0`.
* **Display Output:** Linux Framebuffer device mapped at `/dev/fb0` (Resolution: 1024x600, 16-bit or 32-bit color depth).
* **Touch Input:** Capacitive touch screen configured at `/dev/input/event0` (Requires `EV_ABS` absolute coordinate support).
* **LED Strip:** WS2812/SK6812 LED strip connected via Hardware SPI at `/dev/spidev0.0` (8MHz clock speed).
* **Thermal Sensor:** DS18B20 (1-Wire interface) mapped dynamically via `/sys/bus/w1/devices/28-*`.

##  4. Software Dependencies

This project strictly adheres to **AUTOSAR C++14/17 guidelines** for resource management. Memory is governed by RAII principles and STL containers (`std::vector`, `std::string`); bare `new`/`delete` calls are entirely prohibited to prevent memory fragmentation and leaks.

* **C++ Standard:** C++17
* **Build System:** CMake (Minimum version 3.10)
* **System Libraries:**
  * `libasound2-dev` (Advanced Linux Sound Architecture API)
  * `pthread` (POSIX Threads)
  * `stdc++fs` (C++17 Filesystem)
  * `libfftw3-dev` / `curl` (For advanced monitoring and weather fetching)
  
##  5. Directory Structure & Modularity

The project is modularized to adhere to the Single Responsibility Principle:

/UI: Contains the strictly decoupled Framebuffer memory-mapping driver, non-blocking touch event parser, and stateless .UIRenderer

/MPM (Media Progress Manager): Handles WAV binary decoding, Lock-Free RingBuffers, Goertzel spectrum extraction, and the blocking ALSA Audio Engine.

/Monitor: Background daemons utilizing  to gather real-time system load, 1-Wire sensor temperatures, and network weather.timerfd

/assets: Houses  music binaries..wav

/UI/Renfer: Houses pre-rendered  UI backgrounds loaded into RAM at startup to achieve Zero Disk I/O during runtime..bmp

## 6. Engineering Workflow & Version Control

Our development lifecycle was strictly dictated by our core design requirements, ensuring every architectural decision served the ultimate goal of deterministic real-time performance. We adopted a phased, design-oriented approach:

* **Phase 1: Requirement Analysis & Blueprinting:** Driven by the strict requirements for glitch-free audio and a constant 30FPS UI, we categorically rejected traditional OS polling. The initial design phase established a pure event-driven blueprint mapped directly to Linux kernel interrupts.
* * **Phase 2: Modular System Design (SOLID):** To satisfy the requirement for high maintainability and system stability, the architecture was explicitly decoupled. The Framebuffer GUI, ALSA Audio Engine, and Sensor Monitors were designed as isolated components that communicate exclusively via lock-free data structures.
* * **Phase 3: Hardware-in-the-Loop Prototyping (TDD):** Theoretical designs were immediately tested against physical hardware constraints. Using Test-Driven Development directly on the Raspberry Pi, we continuously verified that our `std::atomic` RingBuffers successfully eliminated mutex bottlenecks under real-world acoustic loads.
* * **Phase 4: Profiling & Iterative Optimization:** To rigorously meet the stringent `<15%` CPU utilization boundary, the system underwent continuous performance profiling. Thread priorities and evdev blocking states were empirically fine-tuned until the latency metrics perfectly aligned with our initial design specifications.

## 7. Team 15 & Contributions
This project was collaboratively engineered by Team 15:

BingXuan Li — Team Leader / System Architect

Yang Zhang — GUI Programmer / Data Systems Developer

MingCong Xu — Audio Systems Engineer / MPM Core Developer

JiaNan Liu — Hardware Programmer / BSP & Driver Integration

ZiKai Ma — Hardware Programmer / BSP & Driver Integration

## 8. Build and Run Instructions

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



> Glasgow, Spring 2026
