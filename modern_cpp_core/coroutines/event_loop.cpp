#include "event_loop.hpp"
#include <unordered_map>
#include <stdexcept>
#include <mutex>
#include <system_error>
#include <cerrno>

#if defined(__linux__) || defined(__gnu_linux__)
#include <sys/epoll.h>
#include <unistd.h>
#else
#error "EventLoop requires Linux epoll!"
#endif

namespace core::coroutines {

struct EventLoop::Impl {
	int epoll_fd{-1};
	bool running{false};
	std::unordered_map<int, std::coroutine_handle<>> timers;
	std::mutex mtx;

	Impl() {
		epoll_fd = epoll_create1(EPOLL_CLOEXEC);
		if (epoll_fd < 0) {
			throw std::system_error(errno, std::generic_category(), "Failed to create epoll");
		}
	}

	~Impl() {
		if (epoll_fd >= 0) {
			close(epoll_fd);
		}
	}
};

EventLoop& EventLoop::get() {
	static EventLoop instance;
	return instance;
}

EventLoop::EventLoop() : m_impl(std::make_unique<Impl>()) {}
EventLoop::~EventLoop() = default;

void EventLoop::register_timer(int timer_fd, std::coroutine_handle<> coro) {
	epoll_event ev{};
	ev.events = EPOLLIN | EPOLLONESHOT;
	ev.data.fd = timer_fd;

	if (epoll_ctl(m_impl->epoll_fd, EPOLL_CTL_ADD, timer_fd, &ev) < 0) {
		throw std::system_error(errno, std::generic_category(), "Failed to add timer to epoll");
	}
	
	std::scoped_lock lock(m_impl->mtx);
	m_impl->timers[timer_fd] = coro;
}

void EventLoop::unregister_timer(int timer_fd) {
	epoll_ctl(m_impl->epoll_fd, EPOLL_CTL_DEL, timer_fd, nullptr);
	
	std::scoped_lock lock(m_impl->mtx);
	m_impl->timers.erase(timer_fd);
}

void EventLoop::run() {
	m_impl->running = true;
	while (m_impl->running) {
		poll();
	}
}

void EventLoop::poll() {
	constexpr int kMaxEvents = 16;
	epoll_event events[kMaxEvents];

	int timeout_ms = m_impl->running ? 10 : 0; 
	
	int num_events = epoll_wait(m_impl->epoll_fd, events, kMaxEvents, timeout_ms);
	
	if (num_events < 0) {
		if (errno == EINTR) return;
		throw std::system_error(errno, std::generic_category(), "epoll_wait failed");
	}

	for (int i = 0; i < num_events; ++i) {
		int fd = events[i].data.fd;
		std::coroutine_handle<> coro;
		
		{
			std::scoped_lock lock(m_impl->mtx);
			auto it = m_impl->timers.find(fd);
			if (it != m_impl->timers.end()) {
				coro = it->second;
				m_impl->timers.erase(it);
			}
		}
		
		if (coro) {
			uint64_t exp;
			if (::read(fd, &exp, sizeof(uint64_t)) < 0) {
				// ignore
			}
			epoll_ctl(m_impl->epoll_fd, EPOLL_CTL_DEL, fd, nullptr);
			coro.resume();
		}
	}
}

void EventLoop::stop() {
	m_impl->running = false;
}

} // namespace core::coroutines

// RISK REVIEW:
// - Pimpl idiom is used safely with std::unique_ptr to hide implementation details.
// - Concurrency: epoll operations are generally thread-safe on the fd, but the internal map is protected by a mutex.
// - Performance: The unordered_map lookup per event might be slightly slower than embedding the pointer directly in the epoll_event struct.
