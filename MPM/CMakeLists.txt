# MPM/CMakeLists.txt

# 1. 查找系统依赖
find_package(ALSA REQUIRED)
find_package(Threads REQUIRED)

# 2. 将音频引擎和硬件效果打包为模块
# 显式列出所有源文件是更专业的做法
add_library(MPM_Module STATIC
    src/RingBuffer.cpp
    src/WavDecoder.cpp
    src/MediaProgressManager.cpp
    src/MusicController.cpp
    src/AudioEngine.cpp
    src/HardwareController.cpp
    src/LedStripController.cpp
)

# 3. 指定头文件路径
# PUBLIC 确保链接这个模块的人也能找到 MPM 的 include 和根目录的 SystemInterfaces.hpp
target_include_directories(MPM_Module PUBLIC 
    include
    ${CMAKE_SOURCE_DIR} 
)

# 4. 链接库
target_link_libraries(MPM_Module PRIVATE ALSA::ALSA Threads::Threads)
