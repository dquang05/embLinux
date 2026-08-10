#include "coroutine_pool.hpp"
#include <new>
#include <cassert>

namespace core::coroutines {

std::byte *CoroutinePoolAllocator::s_memory_buffer = nullptr;
CoroutinePoolAllocator::Block *CoroutinePoolAllocator::s_free_list = nullptr;
std::size_t CoroutinePoolAllocator::s_block_size = 0;
concurrency::Spinlock CoroutinePoolAllocator::s_spinlock;
bool CoroutinePoolAllocator::s_initialized = false;

void CoroutinePoolAllocator::init(std::size_t max_coroutines, std::size_t block_size) {
	std::scoped_lock lock(s_spinlock);
	if (s_initialized) return;

	// Ensure block size is large enough to hold the intrusive pointer and meets alignment
	s_block_size = (block_size < sizeof(Block)) ? sizeof(Block) : block_size;
	s_block_size = (s_block_size + alignof(std::max_align_t) - 1) & ~(alignof(std::max_align_t) - 1);

	// =========================================================================
	// XIN RAM (HEAP ALLOCATION) - CHỈ CHẠY 1 LẦN DUY NHẤT LÚC KHỞI ĐỘNG (BOOT-TIME)
	// =========================================================================
	// Tại đây, chúng ta gọi new[] (malloc) để xin hệ điều hành một mảng nhớ khổng lồ
	// dùng chung cho toàn bộ các coroutines sau này. 
	// Trong suốt phần đời còn lại của ứng dụng (Real-time loops), hàm này sẽ KHÔNG
	// BAO GIỜ được gọi lại nữa. Trình biên dịch C++20 khi sinh Coroutine Frame sẽ lấy
	// bộ nhớ từ mảng tĩnh này qua hàm allocate() ở dưới.
	s_memory_buffer = new std::byte[max_coroutines * s_block_size];
	
	// Khởi tạo danh sách liên kết các block trống (Free List)
	s_free_list = reinterpret_cast<Block *>(s_memory_buffer);
	Block *current = s_free_list;
	
	for (std::size_t i = 1; i < max_coroutines; ++i) {
		current->next = reinterpret_cast<Block *>(s_memory_buffer + i * s_block_size);
		current = current->next;
	}
	current->next = nullptr;
	
	s_initialized = true;
}

void CoroutinePoolAllocator::destroy() noexcept {
	std::scoped_lock lock(s_spinlock);
	if (!s_initialized) return;
	
	delete[] s_memory_buffer;
	s_memory_buffer = nullptr;
	s_free_list = nullptr;
	s_initialized = false;
}

void *CoroutinePoolAllocator::allocate(std::size_t size) {
	std::scoped_lock lock(s_spinlock);
	
	if (!s_initialized) {
		// Fallback or throw? According to rules, missing init is a fatal architectural error.
		throw std::bad_alloc(); 
	}

	if (size > s_block_size) {
		// Frame size exceeds pre-allocated block size
		throw std::bad_alloc();
	}

	if (!s_free_list) {
		// OOM: Pool is exhausted (Too many concurrent coroutines)
		throw std::bad_alloc();
	}

	// O(1) allocation
	Block *block = s_free_list;
	s_free_list = block->next;
	
	return block;
}

void CoroutinePoolAllocator::deallocate(void *ptr, std::size_t /*size*/) noexcept {
	if (!ptr) return;

	std::scoped_lock lock(s_spinlock);
	if (!s_initialized) return;

	// O(1) deallocation: Push block back to the head of the free list
	Block *block = static_cast<Block *>(ptr);
	block->next = s_free_list;
	s_free_list = block;
}

} // namespace core::coroutines
