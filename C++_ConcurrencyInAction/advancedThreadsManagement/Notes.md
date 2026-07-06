> **Note:** This document focuses on advanced thread management architectures (Thread Pools, Work Stealing, Task Dependencies). Fundamental C++ Concurrency concepts (`thread` lifecycle, `std::thread`, `std::mutex`, `std::lock_guard`, etc.) are covered in detail in the [`../threads/`](../threads/) directory.

# 1. Task Queue Supporting `std::future` and Move-only Callables

## The Problem

In a basic Thread Pool, the work queue typically uses `std::function<void()>` to store tasks. However, if we want our Thread Pool to return a `std::future` (allowing the calling thread to wait for and retrieve the result), we must wrap the submitted task in a `std::packaged_task`.

The critical issue is that `std::packaged_task` is a Move-only type (it can be moved, but strictly forbids copying). Meanwhile, `std::function` requires that all objects it stores be Copy-constructible. Consequently, attempting to push a `std::packaged_task` into a standard `std::queue<std::function<void()>>` will result in a compilation error.

## The Solution

We need to design a custom Type-erasure class to act as a wrapper. This class only needs to satisfy two conditions:

- It can wrap any callable object that takes no parameters and returns `void` (since the `std::packaged_task` itself handles the actual return value and transmits it through the `future` channel).
- It fully supports Move semantics while explicitly disabling Copy semantics.

Wrapper design here: [functionWrapper](functionWrapper.hpp)