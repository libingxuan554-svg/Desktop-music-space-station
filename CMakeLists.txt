cmake_minimum_required(VERSION 3.10)
project(DesktopMusicStation)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 1. 查找依赖
find_package(ALSA REQUIRED)
find_package(Threads REQUIRED)

message(STATUS "🛠️ 正在总装空间站...")

# 2. 编译子模块
add_subdirectory(MPM)
add_subdirectory(UI)
# Monitor 目录若只有 cpp/hpp 则直接编译
add_executable(MusicStation main.cpp Monitor/EnvironmentMonitor.cpp)

# 3. 链接所有模块
target_link_libraries(MusicStation PRIVATE 
    MPM_Module 
    UI_Module 
    ALSA::ALSA 
    Threads::Threads
    stdc++fs
)

# 4. 头文件索引
target_include_directories(MusicStation PRIVATE 
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/MPM/include 
    ${CMAKE_SOURCE_DIR}/UI/include
    ${CMAKE_SOURCE_DIR}/Monitor
)

# 5. 🌟 路径自动对齐（加分项）：自动把 assets 拷贝到 build，防止老师跑不起来
add_custom_command(TARGET MusicStation POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/assets ${CMAKE_BINARY_DIR}/assets
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/UI/Renfer ${CMAKE_BINARY_DIR}/UI/Renfer
)
