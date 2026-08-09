#include "thread_affinity.hpp"

#if defined(__linux__) || defined(__gnu_linux__)
#include <pthread.h>
#include <sched.h>
#endif

namespace core::concurrency {

std::expected<void, std::error_code> set_thread_affinity(std::jthread& t, int core_id) noexcept {
#if defined(__linux__) || defined(__gnu_linux__)
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(core_id, &cpuset);

    int rc = pthread_setaffinity_np(t.native_handle(), sizeof(cpu_set_t), &cpuset);
    if (rc != 0) {
        return std::unexpected(std::error_code(rc, std::system_category()));
    }
    return {};
#else
    // Not implemented on non-Linux POSIX
    return std::unexpected(std::error_code(ENOTSUP, std::system_category()));
#endif
}

std::expected<void, std::error_code> set_thread_realtime_priority(std::jthread& t, int priority) noexcept {
#if defined(__linux__) || defined(__gnu_linux__)
    struct sched_param param;
    param.sched_priority = priority;

    int rc = pthread_setschedparam(t.native_handle(), SCHED_FIFO, &param);
    if (rc != 0) {
        return std::unexpected(std::error_code(rc, std::system_category()));
    }
    return {};
#else
    // Not implemented on non-Linux POSIX
    return std::unexpected(std::error_code(ENOTSUP, std::system_category()));
#endif
}

} // namespace core::concurrency
