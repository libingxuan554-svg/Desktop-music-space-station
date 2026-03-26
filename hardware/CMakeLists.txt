cmake_minimum_required(VERSION 3.10)
project(HardwareModule)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(ALSA REQUIRED)

add_library(Hardware_Module
    HardwareController.cpp
    LedStripController.cpp
)

target_include_directories(Hardware_Module PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}
)

target_link_libraries(Hardware_Module PRIVATE
    ALSA::ALSA
    pthread
)
