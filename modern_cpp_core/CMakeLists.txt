cmake_minimum_required(VERSION 3.16)
project(ModernCppCore VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_subdirectory(concurrency)
add_subdirectory(coroutines)
add_subdirectory(data_structures)
add_subdirectory(memory)
add_subdirectory(patterns)
add_subdirectory(utils)
