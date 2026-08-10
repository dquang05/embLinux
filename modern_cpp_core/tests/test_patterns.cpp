#include "../patterns/core_concepts.hpp"
#include "../patterns/data_pipeline.hpp"
#include "test_utils.hpp"
#include <iostream>
#include <vector>
#include <mutex>
#include <shared_mutex>
#include <string>

// Test Concepts
using namespace core::patterns;

static_assert(numeric<int>, "int must be numeric");
static_assert(numeric<float>, "float must be numeric");
static_assert(numeric<double>, "double must be numeric");
static_assert(!numeric<std::string>, "string is not numeric");

static_assert(lockable<std::mutex>, "std::mutex is lockable");
static_assert(shared_lockable<std::shared_mutex>, "std::shared_mutex is shared_lockable");

void test_data_pipeline_success() {
    std::vector<int> raw_data = { 10, 50, 5, 20, 100, 2 };
    
    // Filter < 15 and scale by 2
    auto result = pipeline::process_sensor_data(raw_data, 15, 2);
    
    CORE_ASSERT(result.has_value());
    CORE_ASSERT(result.value().size() == 3);
    
    // Remaining values >= 15 are: 50, 20, 100
    // Scaled by 2: 100, 40, 200
    CORE_ASSERT(result.value()[0] == 100);
    CORE_ASSERT(result.value()[1] == 40);
    CORE_ASSERT(result.value()[2] == 200);
    
    std::cout << "test_data_pipeline_success passed.\n";
}

void test_data_pipeline_errors() {
    std::vector<int> raw_data = { 10, 5, 2 };
    
    // Scale factor 0 should return InvalidScaleFactor
    auto res1 = pipeline::process_sensor_data(raw_data, 0, 0);
    CORE_ASSERT(!res1.has_value());
    CORE_ASSERT(res1.error() == pipeline::PipelineError::InvalidScaleFactor);
    
    // Threshold too high
    auto res2 = pipeline::process_sensor_data(raw_data, 100, 2);
    CORE_ASSERT(!res2.has_value());
    CORE_ASSERT(res2.error() == pipeline::PipelineError::ThresholdTooHigh);
    
    // Empty input
    std::vector<int> empty_data;
    auto res3 = pipeline::process_sensor_data(empty_data, 10, 2);
    CORE_ASSERT(!res3.has_value());
    CORE_ASSERT(res3.error() == pipeline::PipelineError::EmptyInput);

    std::cout << "test_data_pipeline_errors passed.\n";
}

int main() {
    test_data_pipeline_success();
    test_data_pipeline_errors();
    std::cout << "All pattern tests passed.\n";
    return 0;
}
