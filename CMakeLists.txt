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
    audio-engine/AudioEngine.cpp 
    audio-engine/AudioEngine.hpp 
    audio-engine/AudioSource.hpp
)

# Link against ALSA and Pthreads
target_link_libraries(AudioEngine PRIVATE ALSA::ALSA pthread)
target_include_directories(AudioEngine PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

#########################################################################
# Link to the UI section

add_subdirectory(MPM)

# Link to the UI section
add_subdirectory(UI)
target_link_libraries(AudioEngine PRIVATE UI_Module MPM_Module) # 顺手把你连上去
