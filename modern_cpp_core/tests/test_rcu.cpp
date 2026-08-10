#include "../data_structures/rcu.hpp"
#include <iostream>
#include <thread>
#include <vector>
#include <cassert>
#include <atomic>
#include <latch>
#include "test_utils.hpp"

using core::data_structures::rcu_domain;
using core::data_structures::scoped_rcu_reader;
using core::data_structures::RcuPtr;

struct ConfigData {
	int version;
	int payload;
};

/**
 * @brief Tests basic read and update functionality in a single thread.
 */
void test_basic_rcu() {
	RcuPtr<ConfigData> rcu_config(new ConfigData{1, 100});

	{
		scoped_rcu_reader reader;
		const ConfigData* data = rcu_config.read();
		CORE_ASSERT(data != nullptr);
		CORE_ASSERT(data->version == 1);
		CORE_ASSERT(data->payload == 100);
	}

	rcu_config.update(new ConfigData{2, 200});

	{
		scoped_rcu_reader reader;
		const ConfigData* data = rcu_config.read();
		CORE_ASSERT(data != nullptr);
		CORE_ASSERT(data->version == 2);
		CORE_ASSERT(data->payload == 200);
	}
	
	CORE_PASS("test_basic_rcu");
}

/**
 * @brief Tests concurrent read-heavy workload against periodic updates.
 */
void test_concurrent_read_heavy() {
	RcuPtr<ConfigData> rcu_config(new ConfigData{0, 0});
	
	constexpr int kNumReaders = 10;
	constexpr int kReaderOperations = 10000;
	constexpr int kWriterOperations = 50;

	std::atomic<int> sum_payload_read{0};
	std::latch start_latch{kNumReaders + 1};

	auto reader_task = [&rcu_config, &sum_payload_read, &start_latch]() {
		start_latch.arrive_and_wait();
		for (int i = 0; i < kReaderOperations; ++i) {
			scoped_rcu_reader reader;
			const ConfigData* data = rcu_config.read();
			
			// In a real scenario, reader does something with data.
			// We just read it to ensure it's not deleted under us.
			if (data) {
				sum_payload_read.fetch_add(data->payload, std::memory_order_relaxed);
			}
		}
	};

	auto writer_task = [&rcu_config, &start_latch]() {
		start_latch.arrive_and_wait();
		for (int i = 1; i <= kWriterOperations; ++i) {
			// Simulate preparing new data
			ConfigData* new_data = new ConfigData{i, i * 10};
			rcu_config.update(new_data);
			
			// Yield to let readers process
			std::this_thread::yield();
		}
	};

	std::vector<std::jthread> threads;
	threads.emplace_back(writer_task);
	
	for (int i = 0; i < kNumReaders; ++i) {
		threads.emplace_back(reader_task);
	}

	threads.clear(); // Waits for all threads to join
	
	CORE_PASS("test_concurrent_read_heavy");
}

struct MockConfigData {
	static std::atomic<int> destroy_count;
	int value;
	
	MockConfigData(int v) : value(v) {}
	~MockConfigData() {
		destroy_count.fetch_add(1, std::memory_order_relaxed);
	}
};

std::atomic<int> MockConfigData::destroy_count{0};

void test_memory_reclamation() {
	MockConfigData::destroy_count.store(0);
	RcuPtr<MockConfigData> rcu_config(new MockConfigData(1));
	
	std::jthread writer;
	{
		scoped_rcu_reader reader; // Start reading
		const MockConfigData* data = rcu_config.read();
		CORE_ASSERT(data != nullptr);
		CORE_ASSERT(data->value == 1);
		
		std::atomic<bool> writer_started{false};
		
		// Concurrently update it while reader is active
		writer = std::jthread([&rcu_config, &writer_started]() {
			writer_started.store(true, std::memory_order_release);
			rcu_config.update(new MockConfigData(2));
		});
		
		// Wait for writer to at least start and ideally block in synchronize()
		while (!writer_started.load(std::memory_order_acquire)) {
			std::this_thread::yield();
		}
		// Give it a tiny bit of time to reach m_domain.synchronize()
		std::this_thread::sleep_for(std::chrono::milliseconds(50));
		
		// Because the reader is still active, the old data MUST NOT be destroyed yet
		CORE_ASSERT(MockConfigData::destroy_count.load() == 0);
	}
	
	// Reader is now out of scope. 
	// The writer should now be able to finish synchronize() and delete the old data.
	writer.join();
	
	CORE_ASSERT(MockConfigData::destroy_count.load() == 1);
	CORE_PASS("test_memory_reclamation");
}

int main() {
	test_basic_rcu();
	test_concurrent_read_heavy();
	test_memory_reclamation();
	return 0;
}

// RISK REVIEW:
// 1. TSan Validation: TSan must run this test to verify that ConfigData is NEVER 
//    deleted while a reader holds a pointer to it.
