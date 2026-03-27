find_package(ALSA REQUIRED)
find_package(Threads REQUIRED)

# 把音频引擎和同学的硬件灯效打包成一个超级模块
add_library(MPM_Module
    src/RingBuffer.cpp
    src/WavDecoder.cpp
    src/MediaProgressManager.cpp
    src/MusicController.cpp
    src/AudioEngine.cpp
    src/HardwareController.cpp
    src/LedStripController.cpp
)

target_include_directories(MPM_Module PUBLIC include)
target_link_libraries(MPM_Module PRIVATE ALSA::ALSA Threads::Threads)

# ==========================================
# 编译你的主程序 (现在它叫 MusicStation 了！)
# ==========================================
add_executable(MusicStation main.cpp)
target_link_libraries(MusicStation PRIVATE MPM_Module ALSA::ALSA Threads::Threads)
target_include_directories(MusicStation PRIVATE include)
