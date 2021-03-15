cmake_minimum_required(VERSION 3.10)
project(i3-focus-last)

set(CMAKE_CXX_STANDARD 17)

add_subdirectory(i3ipc++)

include_directories(${I3IPCPP_INCLUDE_DIRS})
link_directories(${I3IPCPP_LIBRARY_DIRS})
link_libraries(${I3IPCPP_LIBRARIES})

SET(CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS}-pthread)

add_executable(i3-focus-last main.cpp background.cpp background.h util.h)