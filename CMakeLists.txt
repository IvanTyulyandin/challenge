cmake_minimum_required(VERSION 3.10)
project(challenge)

set(CMAKE_CXX_STANDARD 14)

add_executable(challenge reader.cpp reader.h main.cpp)