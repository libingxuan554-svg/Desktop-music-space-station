# 1. 寻找 Linux 硬件声卡驱动 (接管队友的依赖)
find_package(ALSA REQUIRED)

# 2. 组装你的超级模块 (把你、桥梁、队友的代码全包进去)
add_library(MPM_Module
    src/RingBuffer.cpp
    src/WavDecoder.cpp
    src/MediaProgressManager.cpp
    src/MusicController.cpp       # 我们写的融合桥梁
    src/AudioEngine.cpp           # 刚兼并过来的队友代码
)

# 3. 暴露 include 文件夹，并链接硬件驱动！
target_include_directories(MPM_Module PUBLIC include)
target_link_libraries(MPM_Module PRIVATE ALSA::ALSA pthread)

# ==========================================
# 独立测试程序图纸
# ==========================================
add_executable(TestMPM test_mpm.cpp)
# 测试程序需要链接 MPM_Module，同时由于底层用了 ALSA，为了稳妥加上 ALSA 依赖
target_link_libraries(TestMPM PRIVATE MPM_Module ALSA::ALSA pthread)
