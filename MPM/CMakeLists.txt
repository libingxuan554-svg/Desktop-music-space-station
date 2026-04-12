find_package(ALSA REQUIRED)
find_package(Threads REQUIRED)

# Package the audio engine and the hardware LED effects into one combined module
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
