#include <iostream>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <queue>

std::queue<int> data_queue;
std::mutex mtx;
std::condition_variable cv;

// Producer: Adds data to queue
void producer() {
    for (int i = 0; i < 5; ++i) {
        std::lock_guard<std::mutex> lk(mtx);
        data_queue.push(i);
        std::cout << "Produced: " << i << std::endl;
        cv.notify_one(); // Notify consumer
    }
}

// Consumer: Waits for data
void consumer() {
    while (true) {
        std::unique_lock<std::mutex> lk(mtx);
        cv.wait(lk, []{ return !data_queue.empty(); }); // Wait for condition
        int val = data_queue.front();
        data_queue.pop();
        std::cout << "Consumed: " << val << std::endl;
        if (val == 4) break;
    }
}

int main() {
    std::thread t1(producer);
    std::thread t2(consumer);
    t1.join();
    t2.join();
    return 0;
}