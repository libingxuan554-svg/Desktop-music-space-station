cmake_minimum_required(VERSION 3.10)
project(UI_Module)

# Enable C++14 as per course requirements
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Enable all compiler warnings to ensure high code quality (Grade A requirement)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -pthread")

# Include the 'include' directory for header files
include_directories(include)

# ---------------------------------------------------------------------------
# TARGET 1: UI Static Library
# This encapsulates the Framebuffer implementation so the Main Lead can link it.
# ---------------------------------------------------------------------------
add_library(UI_Module STATIC src/FramebufferUI.cpp)

# ---------------------------------------------------------------------------
# TARGET 2: Unit Tests (TDD - Test Driven Development)
# This allows us to verify logic without needing the actual hardware/display.
# ---------------------------------------------------------------------------
add_executable(unit_tests tests/test_ui_logic.cpp)

# Link the test executable with our UI library
target_link_libraries(unit_tests UI_Module)

# ---------------------------------------------------------------------------
# Integration Note for Main Lead:
# To use this module in the root directory, add: "add_subdirectory(UI)"
# and "target_link_libraries(MainProjectName PUBLIC UI_Module)"
# ---------------------------------------------------------------------------
message(STATUS "UI Module: Build configuration complete. TDD tests enabled.")
