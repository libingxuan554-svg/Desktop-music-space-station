cmake_minimum_required(VERSION 3.10)
project(DesktopMusicStation)

set(CMAKE_CXX_STANDARD 17)

# 打印一下进度信息
message(STATUS "UI Module: Plug-and-Play architecture ready.")
message(STATUS "UI Module: Adheres to SystemInterfaces.hpp protocol.")

# 只需要这一句话：让 CMake 进 MPM 文件夹去编你的超级模块
add_subdirectory(MPM)
