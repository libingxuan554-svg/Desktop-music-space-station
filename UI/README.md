# UI & Interaction Module (Framebuffer Implementation)

## 1. Project Background and Responsibilities
As the **UI & Interaction Developer** of this project team, I am responsible for developing a real-time graphical interface that does not rely on heavyweight frameworks (such as Qt). This module directly operates on the Linux Framebuffer (`/dev/fb0`) to meet the extremely high real-time requirements (Real-time constraints) of embedded systems. 

## 2. Design Philosophy
When writing the code, I referred to the **AUTOSAR C++14** guidelines and the **RT-Embedded** programming standards as specified in the course:
* **Zero Polling**: The UI refresh is entirely based on the callback mechanism. When the `AudioEngine` or `Sensor` thread generates data, my refresh interface is called.
* **Memory Mapping (mmap)**: I abandoned the traditional `write()` system call and instead used memory mapping to reduce latency and ensure that UI refresh does not block the core business logic.
* **Thread Safety**: Considering that audio and hardware data may come from different threads, I used `std::mutex` to protect the video memory, preventing screen tearing.
* **TDD Development Mode**: I first wrote `tests/test_ui_logic.cpp` to verify the mapping logic from intensity to pixels, ensuring that the algorithm was correct before accessing the hardware. 

## 3. Directory Structure
* `include/`: Contains `FramebufferUI.hpp`, providing the external interface and Doxygen comments for the module.
* `src/`: Contains the core implementation `FramebufferUI.cpp`.
* `tests/`: Contains unit test code, reflecting the test-driven development process.
* `CMakeLists.txt`: Automated build script. 

## 4. How to Compile and Run 
Compilation Module ```bash
mkdir build && cd build
cmake ..
make