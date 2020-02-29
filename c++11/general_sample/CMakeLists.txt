cmake_minimum_required(VERSION 2.8)
project(c++)

add_definitions(-std=c++11)
add_executable(hello_c++ main.cpp)
target_link_libraries(hello_c++ pthread)
