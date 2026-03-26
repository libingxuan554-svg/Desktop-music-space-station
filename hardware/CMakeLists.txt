cmake_minimum_required(VERSION 3.10)
project(SpaceStation)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(SpaceStation
    main.cpp
    HardwareController.cpp
    LedStripController.cpp
)

target_link_libraries(SpaceStation
    asound
    pthread
)
