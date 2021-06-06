cmake_minimum_required(VERSION 3.0)
project(TCalc-BadApple)
set(CMAKE_CXX_STANDARD 17)
find_package(OpenCV 3.2 REQUIRED COMPONENTS core highgui imgproc videoio)

set(OPENCV_LIBS opencv_core opencv_highgui opencv_imgproc opencv_videoio)

if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
   add_compile_options (-fdiagnostics-color=always)
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
   add_compile_options (-fcolor-diagnostics)
endif ()

add_executable(vidproc vidproc.cpp)
target_link_libraries(vidproc ${OPENCV_LIBS})

add_executable(vidunproc vidunproc.cpp)
target_link_libraries(vidunproc ${OPENCV_LIBS})
