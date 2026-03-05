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

# --- SDL2 + SDL2_ttf: 用 pkg-config 最稳 ---
find_package(PkgConfig REQUIRED)

# 注意：Debian/RPi 系上 pkg-config 名字通常是 sdl2 / SDL2_ttf
pkg_check_modules(SDL2 REQUIRED sdl2)
pkg_check_modules(SDL2TTF REQUIRED SDL2_ttf)

target_include_directories(music_player PRIVATE
    ${SDL2_INCLUDE_DIRS}
    ${SDL2TTF_INCLUDE_DIRS}
)

target_link_directories(music_player PRIVATE
    ${SDL2_LIBRARY_DIRS}
    ${SDL2TTF_LIBRARY_DIRS}
)

target_link_libraries(music_player PRIVATE
    ${SDL2_LIBRARIES}
    ${SDL2TTF_LIBRARIES}
)

# --- libgpiod ---
target_link_libraries(music_player PRIVATE gpiod)

# --- rpi_ws281x ---
# 有些系统库名是 ws2811，有些需要显式找库路径
find_library(WS2811_LIB ws2811)
if (WS2811_LIB)
  target_link_libraries(music_player PRIVATE ${WS2811_LIB})
else()
  # 兜底：如果 find_library 找不到，你可以手动加库目录
  # 例如：/usr/local/lib
  target_link_directories(music_player PRIVATE /usr/local/lib)
  target_link_libraries(music_player PRIVATE ws2811)
endif()

target_link_libraries(music_player PRIVATE pthread)
