add_executable(test_concurrency test_concurrency.cpp)
target_link_libraries(test_concurrency PRIVATE core_concurrency)
target_compile_options(test_concurrency PRIVATE -Wall -Wextra -Werror)

add_executable(test_coroutines test_coroutines.cpp)
target_link_libraries(test_coroutines PRIVATE core_coroutines)
target_compile_options(test_coroutines PRIVATE -Wall -Wextra -Werror)
