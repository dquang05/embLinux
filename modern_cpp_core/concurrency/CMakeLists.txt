add_library(core_concurrency STATIC thread_pool.cpp)
target_include_directories(core_concurrency PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_options(core_concurrency PRIVATE -Wall -Wextra -Werror)
