cmake_minimum_required(VERSION 3.10)
project(i3-focus-last)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if (CMAKE_BUILD_TYPE STREQUAL "")
    set(CMAKE_BUILD_TYPE "Release")
    message("Using default \"Release\" build type as none was specified.")
endif()

message("Using CMake build type \"${CMAKE_BUILD_TYPE}\" with binary directory \"${CMAKE_BINARY_DIR}\".")

add_subdirectory(i3ipc++)

add_executable(i3-focus-last main.cpp background.cpp background.h util.h)

target_include_directories(i3-focus-last PRIVATE ${I3IPCPP_INCLUDE_DIRS})
target_link_directories(i3-focus-last PRIVATE ${I3IPCPP_LIBRARY_DIRS})
target_link_libraries(i3-focus-last PRIVATE ${I3IPCPP_LIBRARIES})

SET(CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS}-pthread)
