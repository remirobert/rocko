cmake_minimum_required(VERSION 3.5)
project(rocko C)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

set(SOURCE_FILES main.c rocko.c rocko.h)
add_executable(rocko ${SOURCE_FILES})