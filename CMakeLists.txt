cmake_minimum_required(VERSION 3.16)
project(DesktopMusicStation_UI)

# Set C++ Standard for deterministic behavior
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find GUI Dependencies (Qt6)
find_package(Qt6 REQUIRED COMPONENTS Widgets)

# Include directories to link UIHandler.hpp
include_directories(include)

# Source files list
set(SOURCES 
    src/main.cpp
    # Add StandbyView.cpp and MusicView.cpp here as you create them
)

add_executable(UI_Module ${SOURCES})

# Link libraries to the UI executable
target_link_libraries(UI_Module PRIVATE Qt6::Widgets)
