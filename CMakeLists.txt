cmake_minimum_required(VERSION 3.10)
project(challenge)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O2 -march=native -pthread")
add_executable(challenge reader.cpp reader.h main.cpp)