cmake_minimum_required(VERSION 3.10)
project(AudioEngineModule)

# C++17 for better atomic and thread support
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Enable aggressive optimizations for performance
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -O3")

# Find the ALSA dependency
find_package(ALSA REQUIRED)

# Define the library for the project
add_library(AudioEngine 
    AudioEngine.cpp 
    AudioEngine.hpp 
    AudioSource.hpp
)

# Link against ALSA and Pthreads
target_link_libraries(AudioEngine PRIVATE ALSA::ALSA pthread)
target_include_directories(AudioEngine PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

#########################################################################
# 1. 注册你的 MPM 模块文件夹
add_subdirectory(MPM)

# 2. 注册 UI 模块文件夹 (保持原样)
add_subdirectory(UI)

# 3. 终极缝合！把你的 MPM_Module 和 UI_Module 像装配零件一样接到主引擎上
target_link_libraries(AudioEngine 
    PRIVATE 
    MPM_Module 
    UI_Module
)
