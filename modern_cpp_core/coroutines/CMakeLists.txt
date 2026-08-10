add_library(core_coroutines STATIC 
    coroutine_pool.cpp 
    event_loop.cpp
)
target_include_directories(core_coroutines PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_options(core_coroutines PRIVATE -Wall -Wextra -Werror)
target_link_libraries(core_coroutines PUBLIC core_concurrency)
