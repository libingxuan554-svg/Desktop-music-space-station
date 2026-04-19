cmake_minimum_required(VERSION 3.10)
project(DesktopMusicStation)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 1. Search for dependencies
find_package(ALSA REQUIRED)
find_package(Threads REQUIRED)

message(STATUS "The space station is currently in the final assembly stage....")

# 2. Compilation sub-module
add_subdirectory(MPM)
add_subdirectory(UI)
# If the Monitor directory only contains cpp/hpp files, it will be compiled directly.
add_executable(MusicStation main.cpp Monitor/EnvironmentMonitor.cpp)

# 3. comnine all modules
target_link_libraries(MusicStation PRIVATE 
    MPM_Module 
    UI_Module 
    ALSA::ALSA 
    Threads::Threads
    stdc++fs
)

# 4. Header file index
target_include_directories(MusicStation PRIVATE 
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/MPM/include 
    ${CMAKE_SOURCE_DIR}/UI/include
    ${CMAKE_SOURCE_DIR}/Monitor
)

# 5. Path automatic alignment
add_custom_command(TARGET MusicStation POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/assets ${CMAKE_BINARY_DIR}/assets
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/UI/Renfer ${CMAKE_BINARY_DIR}/UI/Renfer
)
