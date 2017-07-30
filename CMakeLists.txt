cmake_minimum_required(VERSION 3.8)
project(heart)

set(CMAKE_CXX_STANDARD 14)

add_library(heart INTERFACE)
target_include_directories(heart INTERFACE include/)

add_executable(heart_example example/main.cpp)
target_link_libraries(heart_example heart)