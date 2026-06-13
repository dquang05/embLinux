cmake_minimum_required(VERSION 3.10)
project(TLPI_Master_Practice C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Go up one level from 'syntax' to 'LSPI', then enter the 'lib' directory
set(LIB_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../lib")
include_directories(${LIB_DIR})

set(TLPI_LIB_SRCS ${LIB_DIR}/error_functions.c ${LIB_DIR}/get_num.c)

# Example 4: IPC
add_executable(mmap 
    "${CMAKE_CURRENT_SOURCE_DIR}/04_ipc/memoryMapping/cat.c" 
    ${TLPI_LIB_SRCS}
)

target_include_directories(mmap PRIVATE ${LIB_DIR})