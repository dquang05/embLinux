

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

add_executable(test_coroutines_advanced test_coroutines_advanced.cpp)
target_link_libraries(test_coroutines_advanced PRIVATE core_coroutines)
target_compile_options(test_coroutines_advanced PRIVATE -Wall -Wextra -Werror)

# Add CTest support if not already there
enable_testing()
add_test(NAME JThreadPoolTest COMMAND test_jthread_pool)
add_test(NAME RemainingModule1Test COMMAND test_remaining_module1)
add_test(NAME CoroutinesAdvancedTest COMMAND test_coroutines_advanced)

add_test(NAME LockFreeStackTest COMMAND test_lock_free_stack)
add_test(NAME LockFreeQueueTest COMMAND test_lock_free_queue)
add_test(NAME ThreadSafeQueueTest COMMAND test_thread_safe_queue)
add_test(NAME ThreadSafeHashMapTest COMMAND test_thread_safe_hash_map)

add_executable(test_rcu test_rcu.cpp)
target_link_libraries(test_rcu PRIVATE core_data_structures)
target_compile_options(test_rcu PRIVATE -Wall -Wextra -Werror -Wno-interference-size)
add_test(NAME RcuTest COMMAND test_rcu)

add_executable(test_aligned_cache_line test_aligned_cache_line.cpp)
target_compile_options(test_aligned_cache_line PRIVATE -Wall -Wextra -Werror -Wno-interference-size)
add_test(NAME AlignedCacheLineTest COMMAND test_aligned_cache_line)

add_executable(test_atomic_ref test_atomic_ref.cpp)
target_compile_options(test_atomic_ref PRIVATE -Wall -Wextra -Werror)
add_test(NAME AtomicRefTest COMMAND test_atomic_ref)

add_executable(test_monotonic_memory_pool test_monotonic_memory_pool.cpp)
target_compile_options(test_monotonic_memory_pool PRIVATE -Wall -Wextra -Werror)
add_test(NAME MonotonicMemoryPoolTest COMMAND test_monotonic_memory_pool)

add_executable(test_constinit test_constinit.cpp)
target_compile_options(test_constinit PRIVATE -Wall -Wextra -Werror)
add_test(NAME ConstinitTest COMMAND test_constinit)

add_executable(test_atomic_float test_atomic_float.cpp)
target_compile_options(test_atomic_float PRIVATE -Wall -Wextra -Werror)
add_test(NAME AtomicFloatTest COMMAND test_atomic_float)

add_executable(test_patterns test_patterns.cpp)
target_link_libraries(test_patterns PRIVATE core_patterns)
target_compile_options(test_patterns PRIVATE -Wall -Wextra -Werror)
add_test(NAME PatternsTest COMMAND test_patterns)

add_executable(test_parallel test_parallel.cpp)
target_link_libraries(test_parallel PRIVATE core_patterns core_concurrency)
target_compile_options(test_parallel PRIVATE -Wall -Wextra -Werror)
add_test(NAME ParallelTest COMMAND test_parallel)

add_executable(test_hazard_pointer test_hazard_pointer.cpp)
target_link_libraries(test_hazard_pointer PRIVATE core_data_structures)
target_compile_options(test_hazard_pointer PRIVATE -Wall -Wextra -Werror)
add_test(NAME HazardPointerTest COMMAND test_hazard_pointer)

set_tests_properties(
    JThreadPoolTest RemainingModule1Test CoroutinesAdvancedTest
    LockFreeStackTest LockFreeQueueTest ThreadSafeQueueTest
    ThreadSafeHashMapTest RcuTest AlignedCacheLineTest
    AtomicRefTest MonotonicMemoryPoolTest ConstinitTest
    AtomicFloatTest PatternsTest ParallelTest HazardPointerTest
    PROPERTIES TIMEOUT 10
)
