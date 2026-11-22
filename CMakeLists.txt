cmake_minimum_required(VERSION 3.23)
project(MicroMetro)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(SDL3 CONFIG REQUIRED)

add_executable(MicroMetro src/main.cpp)

target_link_libraries(MicroMetro SDL3::SDL3)