cmake_minimum_required (VERSION 2.8.11)
project (IR)

set(CMAKE_CXX_FLAGS "-pedantic-errors -std=c++11 -pthread -g -O3 -Wall")

file(GLOB SOURCES "*.hpp")
file(GLOB SOURCES "*.cpp")

add_executable (IR ${SOURCES})