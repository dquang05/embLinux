#pragma once

#include <array>
#include <list>
#include <optional>
#include <shared_mutex>
#include <mutex>

namespace core::data_structures {

/**
 * @brief A thread-safe hash map with fine-grained locking and no automatic rehashing.
 * 
 * @tparam K The key type.
 * @tparam V The value type.
 * @tparam NumBuckets The fixed number of buckets (defaults to a prime number).
 */
template <typename K, typename V, size_t NumBuckets = 31>
class ThreadSafeHashMap {
private:
	struct Bucket {
		std::list<std::pair<K, V>> data;
		mutable std::shared_mutex mutex;
	};

	std::array<Bucket, NumBuckets> m_buckets;

	/**
	 * @brief Hashes the key and returns the corresponding bucket.
	 */
	Bucket &get_bucket(const K &key) const {
		std::hash<K> hasher;
		size_t bucket_index = hasher(key) % NumBuckets;
		return const_cast<Bucket &>(m_buckets[bucket_index]);
	}

public:
	ThreadSafeHashMap() = default;
	~ThreadSafeHashMap() = default;

	ThreadSafeHashMap(const ThreadSafeHashMap &) = delete;
	ThreadSafeHashMap &operator=(const ThreadSafeHashMap &) = delete;

	/**
	 * @brief Inserts or updates a key-value pair.
	 * 
	 * @param key The key to insert/update.
	 * @param value The associated value.
	 */
	void insert(const K &key, const V &value) {
		Bucket &bucket = get_bucket(key);
		std::unique_lock lock(bucket.mutex);
		for (auto &pair : bucket.data) {
			if (pair.first == key) {
				pair.second = value;
				return;
			}
		}
		bucket.data.emplace_back(key, value);
	}

	/**
	 * @brief Retrieves the value associated with the key.
	 * 
	 * @param key The key to search for.
	 * @return std::optional<V> The value, or std::nullopt if not found.
	 */
	std::optional<V> get(const K &key) const {
		Bucket &bucket = get_bucket(key);
		std::shared_lock lock(bucket.mutex);
		for (const auto &pair : bucket.data) {
			if (pair.first == key) {
				return pair.second;
			}
		}
		return std::nullopt;
	}

	/**
	 * @brief Removes the key-value pair from the map.
	 * 
	 * @param key The key to remove.
	 */
	void remove(const K &key) {
		Bucket &bucket = get_bucket(key);
		std::unique_lock lock(bucket.mutex);
		bucket.data.remove_if([&key](const std::pair<K, V> &pair) {
			return pair.first == key;
		});
	}
};

// RISK REVIEW:
// 1. Concurrency: Uses fine-grained locking (std::shared_mutex per bucket). Multiple threads can 
//    read (get) concurrently from the same bucket, and multiple threads can write (insert/remove) 
//    concurrently as long as they hash to different buckets.
// 2. Caller responsibilities: Hash function for K must be well-distributed to avoid bucket collisions 
//    which would degrade concurrent performance.
// 3. Unspecified edge cases: No automatic rehashing. If many elements hash to the same bucket, 
//    performance degrades from O(1) to O(N) where N is the bucket size.

} // namespace core::data_structures
