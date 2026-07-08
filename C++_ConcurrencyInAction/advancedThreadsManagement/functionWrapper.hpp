#pragma once
#include <memory>
#include <utility>
// Depricated in C++17
class function_wrapper {
    struct impl_base {
        virtual void call() = 0;
        virtual ~impl_base() {}
    };

    std::unique_ptr<impl_base> impl;

    template<typename F>
    struct impl_type : impl_base {
        F f;
        
        // Constructor using move semantics to construct the callable
        impl_type(F&& f_) : f(std::move(f_)) {}
        
        // Execute the underlying task
        void call() override { f(); }
    };

public:
    // Default constructor
    function_wrapper() = default;

    // Template constructor to accept any callable object (like std::packaged_task)
    template<typename F>
    function_wrapper(F&& f) : impl(new impl_type<F>(std::move(f))) {}

    // Enable move semantics
    function_wrapper(function_wrapper&& other) noexcept : impl(std::move(other.impl)) {}
    
    function_wrapper& operator=(function_wrapper&& other) noexcept {
        impl = std::move(other.impl);
        return *this;
    }

    // Explicitly delete copy semantics to prevent compilation errors 
    // when dealing with move-only types like std::packaged_task
    function_wrapper(const function_wrapper&) = delete;
    function_wrapper(function_wrapper&) = delete;
    function_wrapper& operator=(const function_wrapper&) = delete;

    // Function call operator to execute the stored task
    void operator()() {
        if (impl) {
            impl->call();
        }
    }
};