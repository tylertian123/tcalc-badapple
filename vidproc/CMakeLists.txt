cmake_minimum_required(VERSION 3.0)
project(TCalc-BadApple)
set(CMAKE_CXX_STANDARD 17)
find_package(OpenCV REQUIRED)
include_directories(${OpenCV_INCLUDE_DIRS})

add_executable(vidproc vidproc.cpp)
target_link_libraries(vidproc ${OpenCV_LIBS})
