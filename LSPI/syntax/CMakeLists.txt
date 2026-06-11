cmake_minimum_required(VERSION 3.10)
project(TLPI_Master_Practice C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(LIB_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../lib")
include_directories(${LIB_DIR})

set(TLPI_LIB_SRCS ${LIB_DIR}/error_functions.c ${LIB_DIR}/get_num.c)



# Example1: IO
# add_executable(01_io_main 01_io/main.c ${TLPI_LIB_SRCS})

# Example 2: Process 
add_executable(02_process 02_process/miniShell.c ${TLPI_LIB_SRCS})

# Example 3: Thread
# add_executable(03_thread_demo 03_thread/thread_demo.c ${TLPI_LIB_SRCS})
