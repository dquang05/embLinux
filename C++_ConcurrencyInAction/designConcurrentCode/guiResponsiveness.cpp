#include <iostream>
#include <thread>
#include <atomic>
#include <chrono>

// Atomic flag used for cross-thread communication (Thread-safe)
std::atomic<bool> task_cancelled(false);
std::atomic<bool> task_complete(false);

// The heavy background task
void background_task() {
    std::cout << "[Task Thread] Background processing started...\n";
    
    int progress = 0;
    // Loop continues until done OR the GUI thread requests cancellation
    while (progress < 10 && !task_cancelled.load()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(500)); // Simulate heavy work
        progress++;
        std::cout << "[Task Thread] Progress: " << progress * 10 << "%\n";
    }

    if (task_cancelled.load()) {
        std::cout << "[Task Thread] Task was gracefully cancelled. Cleaning up...\n";
    } else {
        std::cout << "[Task Thread] Task completed successfully!\n";
        task_complete.store(true);
    }
}

// The GUI Event Loop
void gui_thread() {
    std::thread worker;

    std::cout << "[GUI] App Started. Simulating user input...\n";
    std::cout << "[GUI] User clicked 'Start Task'\n";
    
    // Offload heavy work to background thread
    task_cancelled.store(false);
    task_complete.store(false);
    worker = std::thread(background_task);

    // GUI remains fully responsive!
    for (int i = 0; i < 3; ++i) {
        std::this_thread::sleep_for(std::chrono::milliseconds(700));
        std::cout << "[GUI] Handling UI rendering and mouse movements smoothly...\n";
    }

    std::cout << "[GUI] User clicked 'Cancel Task'\n";
    task_cancelled.store(true); // Signal the worker to stop

    // Wait for worker to finish cleanup
    if (worker.joinable()) {
        worker.join();
    }

    std::cout << "[GUI] Application closing safely.\n";
}

int main() {
    // Run the main GUI loop
    gui_thread();
    return 0;
}