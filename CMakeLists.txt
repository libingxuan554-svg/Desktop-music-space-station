cmake_minimum_required(VERSION 3.10)
project(DesktopMusicStation)

set(CMAKE_CXX_STANDARD 17)

# 1. 查找系统依赖
find_package(ALSA REQUIRED)
find_package(Threads REQUIRED)

message(STATUS " 准备开始总装空间站...")

# 2. 引入两个零件厂 (执行各自目录的 CMakeLists)
add_subdirectory(MPM)
add_subdirectory(UI)

# 3. 总装厂开始工作：在根目录使用 main.cpp 生成主程序
add_executable(MusicStation main.cpp Monitor/EnvironmentMonitor.cpp)

# 4. 把零件和系统库“焊接”到主程序上
target_link_libraries(MusicStation PRIVATE MPM_Module UI_Module ALSA::ALSA Threads::Threads)

# 5. 告诉 main.cpp 去哪里找头文件 (包含了根目录、MPM的include 和 UI的include)
target_include_directories(MusicStation PRIVATE 
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/MPM/include 
    ${CMAKE_SOURCE_DIR}/UI/include
    ${CMAKE_SOURCE_DIR}/Monitor
)
