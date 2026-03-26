cmake_minimum_required(VERSION 3.10)
project(UI_Module)

# Enable C++17 as per course requirements
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Enable all compiler warnings to ensure high code quality 
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -pthread")

# Include the 'include' directory for header files
include_directories(include)

# ---------------------------------------------------------------------------
# TARGET 1: UI Static Library
# It contains the low-level driver FramebufferUI and high-level renderer UIRenderer
# ---------------------------------------------------------------------------
add_library(UI_Module STATIC 
	src/FramebufferUI.cpp
	src/UIRenderer.cpp
	src/InteractionManager.cpp  # 新增
                  src/TouchHandler.cpp        # 新增
)
target_link_libraries(UI_Module PUBLIC pthread)

# ---------------------------------------------------------------------------
# TARGET 2: Unit Tests (TDD - Test Driven Development)
# This allows us to verify logic without needing the actual hardware/display.
# ---------------------------------------------------------------------------
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/tests/test_ui_logic.cpp")
	add_executable(unit_tests tests/test_ui_logic.cpp)

# Link the test executable with our UI library
	target_link_libraries(unit_tests UI_Module pthread)
endif()

# ---------------------------------------------------------------------------
# Integration Note for Main Lead:
# To use this module in the root directory, add: "add_subdirectory(UI)"
# and "target_link_libraries(MainProjectName PUBLIC UI_Module)"
# ---------------------------------------------------------------------------
message(STATUS "UI Module: Plug-and-Play architecture ready.")
message(STATUS "UI Module: Adheres to SystemInterfaces.hpp protocol.")