cmake_minimum_required(VERSION 3.5)
project(SocketTest)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

set(SOURCE_FILES main.c server.c request.c response.c server.h route.c)
add_executable(SocketTest ${SOURCE_FILES})