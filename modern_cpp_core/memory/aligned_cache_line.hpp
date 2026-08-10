#pragma once

#include <new>
#include <utility>

namespace core::memory {

#ifdef __cpp_lib_hardware_interference_size
	constexpr std::size_t kCacheLineSize = std::hardware_destructive_interference_size;
#else
	constexpr std::size_t kCacheLineSize = 64;
#endif

/**
 * @brief A wrapper that aligns the stored value to the hardware cache line size 
 *        to prevent False Sharing in concurrent environments.
 * 
 * @tparam T The type of the value to be stored and aligned.
 */
template <typename T>
struct alignas(kCacheLineSize) AlignedCacheLine {
	T m_value;

	AlignedCacheLine() = default;

	template <typename... Args>
	explicit AlignedCacheLine(Args&&... args) : m_value(std::forward<Args>(args)...) {}

	AlignedCacheLine(const AlignedCacheLine&) = default;
	AlignedCacheLine& operator=(const AlignedCacheLine&) = default;
	AlignedCacheLine(AlignedCacheLine&&) = default;
	AlignedCacheLine& operator=(AlignedCacheLine&&) = default;

	AlignedCacheLine& operator=(const T& v) { m_value = v; return *this; }
	AlignedCacheLine& operator=(T&& v) { m_value = std::move(v); return *this; }

	// Operator overloads for convenience
	operator T&() { return m_value; }
	operator const T&() const { return m_value; }
	
	T* operator&() { return &m_value; }
	const T* operator&() const { return &m_value; }
};

// RISK REVIEW:
// 1. Memory Overhead: Forces alignment which increases memory footprint by creating padding.
//    Should only be used for highly contended variables (e.g. atomics, head/tail indices), 
//    not for arrays of data.
// 2. Compatibility: Uses C++17 `std::hardware_destructive_interference_size` if available, 
//    falls back to 64 bytes.

} // namespace core::memory
