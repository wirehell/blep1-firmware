cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(openp1_tests)

SET(CMAKE_C_FLAGS "-O0")
SET(CMAKE_CPP_FLAGS "-O0")
set(CMAKE_BUILD_TYPE Debug)

FILE(GLOB app_sources src/*.c)
FILE(GLOB lib_sources ../src/lib/*.c)

target_include_directories(app PRIVATE ../src)

target_sources(app PRIVATE ${app_sources})
target_sources(app PRIVATE ${lib_sources})
