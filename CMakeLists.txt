cmake_minimum_required(VERSION 3.16)
project(MusicPlayer VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

add_executable(audio_test
    main.cpp
    AudioEngine.cpp
)

target_include_directories(audio_test PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_compile_options(audio_test PRIVATE
    -Wall
    -Wextra
    -Wpedantic
)

find_package(Threads REQUIRED)

target_link_libraries(audio_test PRIVATE
    Threads::Threads
    dl
    m
    asound
)
