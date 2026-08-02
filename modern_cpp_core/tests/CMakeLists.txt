add_executable(test_concurrency test_concurrency.cpp)
target_link_libraries(test_concurrency PRIVATE core_concurrency)
target_compile_options(test_concurrency PRIVATE -Wall -Wextra -Werror)
