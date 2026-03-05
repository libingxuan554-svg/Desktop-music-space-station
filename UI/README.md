# UI & Interaction Module (Decoupled Framebuffer Implementation)

## 1. Project Background and Responsibilities
As the UI & Interaction Developer, I have implemented a high-performance graphical interface using the Linux Framebuffer (). This module is designed to handle real-time visualizations for audio intensity and sensor data without the overhead of heavy frameworks like Qt. 

## 2. Design Philosophy
When writing the code, I referred to the **AUTOSAR C++17** guidelines and the **RT-Embedded** programming standards as specified in the course:
* **Zero Polling**: The UI refresh is entirely based on the callback mechanism. When the `AudioEngine` or `Sensor` thread generates data, my refresh interface is called.
* **Memory Mapping (mmap)**: I abandoned the traditional `write()` system call and instead used memory mapping to reduce latency and ensure that UI refresh does not block the core business logic.
* **Thread Safety**: Considering that audio and hardware data may come from different threads, I used `std::mutex` to protect the video memory, preventing screen tearing.
* **TDD Development Mode**: I first wrote `tests/test_ui_logic.cpp` to verify the mapping logic from intensity to pixels, ensuring that the algorithm was correct before accessing the hardware. 

## 3.Decoupled Architecture Design
To ensure code maintainability and scalability, I have adopted a Decoupled Strategy Pattern:
* **Core Driver (FramebufferUI): Responsible for low-level hardware abstraction. It handles  memory mapping, ioctl calls, and provides raw drawing primitives.
* **Visual Renderer (UIRenderer): A static library that contains high-level visual "strategies." It defines how the UI looks (e.g., segmented bars, mirrored equalizers) by calling the driver's primitives.

## 4. Directory Structure
* 'include/FramebufferUI.hpp: Hardware driver interface.
* 'include/UIRenderer.hpp: UI pattern and visual logic definitions.
* 'src/FramebufferUI.cpp: Implementation of mmap and raw pixel operations.
* 'src/UIRenderer.cpp: Implementation of specific visual designs (e.g., tech-bars).
* 'tests/test_ui_logic.cpp: Unit tests for rendering math and boundary checks.
* 'CMakeLists.txt: Automated build script for both the library and unit tests.

## 5. How to Compile and Run 
Compilation Module ```bash
mkdir build && cd build
cmake ..
make