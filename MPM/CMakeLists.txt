# ==========================================
# @brief Media Processing Module (MPM) Build Script
# @note [Architecture / SOLID]:
# - Modularity and Decoupling: Packages the low-level audio engine and hardware control into an independent static library (STATIC), achieving physical isolation from the UI rendering layer, adhering to high cohesion and low coupling.
# ==========================================

# ==========================================
#  Find and link low-level system dependencies
# @note [Real-Time Constraints]:
# # [REAL-TIME COMPLIANCE]:
# # Mandates dependency on ALSA audio library and POSIX threads, providing OS-level support for low-level hardware interrupts, DMA transfers, and zero-polling multi-threaded architecture.
# ==========================================
# 1 1. Find system dependencies
find_package(ALSA REQUIRED)    # Find ALSA (Advanced Linux Sound Architecture) core driver library
find_package(Threads REQUIRED) #  Find system multi-threading library support (pthread)

# ==========================================
# Compile audio engine and hardware effect source files into a static module
# ==========================================
#  2. Package audio engine and hardware effects into a module
# Explicitly listing all source files is a more professional practice, ensuring absolute determinism for cross-platform builds
add_library(MPM_Module STATIC
    src/RingBuffer.cpp           # Lock-free high-frequency ring buffer implementation
    src/WavDecoder.cpp           #  WAV audio binary decoder
    src/MediaProgressManager.cpp # Media progress and timeline manager
    src/MusicController.cpp      #  Music playback state machine controller
    src/AudioEngine.cpp          # Core audio processing and ALSA hardware communication engine
    src/HardwareController.cpp   #  Physical hardware sensor and actuator main controller
    src/LedStripController.cpp   # WS2812B SPI WS2812B SPI ambient LED strip driver layer
)

# ==========================================
# Specify header file exposure paths for the static library
# ==========================================
# 3. Specify header file paths
# PUBLIC ensures that whoever links this module can also find MPM's include and the root directory's SystemInterfaces.hpp
target_include_directories(MPM_Module PUBLIC 
    include              # Expose the internal high-level interface directory of this module
    ${CMAKE_SOURCE_DIR}  # SystemInterfaces.hpp | Expose the project root directory to include the global system protocol SystemInterfaces.hpp
)

# ==========================================
# Link system dynamic runtime libraries to the target module
# ==========================================
#4. Link libraries
target_link_libraries(MPM_Module PRIVATE ALSA::ALSA Threads::Threads) # Link ALSA and multi-threading internally to the MPM module with private permission
