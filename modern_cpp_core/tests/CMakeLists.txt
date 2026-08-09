option(ENABLE_TSAN "Enable ThreadSanitizer" OFF)

if(ENABLE_TSAN)
	message(STATUS "ThreadSanitizer is ENABLED for tests")
	add_compile_options(-fsanitize=thread -fPIE -pie -g -O1)
	add_link_options(-fsanitize=thread -pie)
endif()

# add_executable(test_concurrency test_concurrency.cpp)
# target_link_libraries(test_concurrency PRIVATE core_concurrency)
# target_compile_options(test_concurrency PRIVATE -Wall -Wextra -Werror -Wno-interference-size)

add_executable(test_coroutines test_coroutines.cpp)
target_link_libraries(test_coroutines PRIVATE core_coroutines)
target_compile_options(test_coroutines PRIVATE -Wall -Wextra -Werror -Wno-interference-size)

add_executable(test_lock_free_stack test_lock_free_stack.cpp)
target_link_libraries(test_lock_free_stack PRIVATE core_data_structures)
target_compile_options(test_lock_free_stack PRIVATE -Wall -Wextra -Werror -Wno-interference-size)

add_executable(test_thread_safe_queue test_thread_safe_queue.cpp)
target_link_libraries(test_thread_safe_queue PRIVATE core_data_structures)
target_compile_options(test_thread_safe_queue PRIVATE -Wall -Wextra -Werror -Wno-interference-size)

add_executable(test_lock_free_queue test_lock_free_queue.cpp)
target_link_libraries(test_lock_free_queue PRIVATE core_data_structures)
target_compile_options(test_lock_free_queue PRIVATE -Wall -Wextra -Werror -Wno-interference-size)

add_executable(test_thread_safe_hash_map test_thread_safe_hash_map.cpp)
target_link_libraries(test_thread_safe_hash_map PRIVATE core_data_structures)
target_compile_options(test_thread_safe_hash_map PRIVATE -Wall -Wextra -Werror -Wno-interference-size)

add_executable(test_jthread_pool test_jthread_pool.cpp)
target_link_libraries(test_jthread_pool PRIVATE core_concurrency)
target_compile_options(test_jthread_pool PRIVATE -Wall -Wextra -Werror)

add_executable(test_remaining_module1 test_remaining_module1.cpp)
target_link_libraries(test_remaining_module1 PRIVATE core_concurrency)
target_compile_options(test_remaining_module1 PRIVATE -Wall -Wextra -Werror)

# Add CTest support if not already there
enable_testing()
add_test(NAME JThreadPoolTest COMMAND test_jthread_pool)
add_test(NAME RemainingModule1Test COMMAND test_remaining_module1)
