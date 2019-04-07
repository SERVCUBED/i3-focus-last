cmake_minimum_required(VERSION 3.10)
project(i3_focus_last)

set(CMAKE_CXX_STANDARD 14)

add_subdirectory(i3ipc++)

include_directories(${I3IPCpp_INCLUDE_DIRS})
link_directories(${I3IPCpp_LIBRARY_DIRS})
link_libraries(${I3IPCpp_LIBRARIES})

SET(CMAKE_CXX_FLAGS -pthread)

add_executable(i3_focus_last main.cpp)