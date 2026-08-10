#pragma once

#include <ranges>
#include <vector>
#include <concepts>
#include <expected>
#include <system_error>
#include "core_concepts.hpp"

namespace core::patterns::pipeline {

/**
 * @brief Error types for data pipeline processing.
 */
enum class PipelineError {
    InvalidScaleFactor,
    EmptyInput,
    ThresholdTooHigh
};

/**
 * @brief Processes raw sensor data by filtering out noise and scaling it.
 * Utilizes C++20 ranges to process data efficiently without creating 
 * intermediate temporary containers for each step.
 * 
 * @tparam Range The input range type.
 * @param input The raw sensor data.
 * @param noise_threshold Values strictly below this are considered noise and filtered out.
 * @param scale_factor Factor to multiply the remaining data points by.
 * @return std::expected containing the processed vector, or an error code.
 */
template <std::ranges::input_range Range>
requires numeric<std::ranges::range_value_t<Range>>
std::expected<std::vector<std::ranges::range_value_t<Range>>, PipelineError>
process_sensor_data(Range&& input, 
                    std::ranges::range_value_t<Range> noise_threshold, 
                    std::ranges::range_value_t<Range> scale_factor) {
    
    using T = std::ranges::range_value_t<Range>;

    // Validate parameters
    if (scale_factor == 0) {
        return std::unexpected(PipelineError::InvalidScaleFactor);
    }
    
    // C++20 Ranges Data Pipeline
    // Notice how we chain operations without creating temporary vectors
    auto pipeline = input 
                  | std::views::filter([noise_threshold](const T& val) { return val >= noise_threshold; })
                  | std::views::transform([scale_factor](const T& val) { return val * scale_factor; });
                  
    std::vector<T> result;
    
    // Execute the lazy-evaluated pipeline
    for (auto val : pipeline) {
        result.push_back(val);
    }
    
    if (result.empty()) {
        // Did we filter everything out or was input empty?
        if (std::ranges::empty(input)) {
            return std::unexpected(PipelineError::EmptyInput);
        } else {
            return std::unexpected(PipelineError::ThresholdTooHigh);
        }
    }
    
    return result;
}

} // namespace core::patterns::pipeline
