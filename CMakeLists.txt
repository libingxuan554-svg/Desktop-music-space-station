cmake_minimum_required(VERSION 3.16)
project(music_player CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(music_player
    src/main.cpp
    src/Player.cpp
    src/UI.cpp
    src/PIR.cpp
    src/LED.cpp
)

target_include_directories(music_player PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party
)

find_package(PkgConfig REQUIRED)
pkg_check_modules(SDL2 REQUIRED sdl2)
pkg_check_modules(SDL2TTF REQUIRED SDL2_ttf)

target_include_directories(music_player PRIVATE
    ${SDL2_INCLUDE_DIRS}
    ${SDL2TTF_INCLUDE_DIRS}
)

target_link_libraries(music_player PRIVATE
    ${SDL2_LIBRARIES}
    ${SDL2TTF_LIBRARIES}
    gpiod
    ws2811
    pthread
)
