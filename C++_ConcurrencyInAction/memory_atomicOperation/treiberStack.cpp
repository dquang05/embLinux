#include <atomic>
#include <iostream>
#include <thread>

template<typename T>
class LockFreeStack {
private:
    struct Node {
        T data;
        Node* next;
        Node(const T& data_) : data(data_), next(nullptr) {}
    };

    std::atomic<Node*> head;

public:
    LockFreeStack() : head(nullptr) {}

    void push(const T& data) {
        Node* new_node = new Node(data);

        // Load current head. We use relaxed because we just need the pointer value.
        // The actual synchronization happens during the compare_exchange.
        new_node->next = head.load(std::memory_order_relaxed);

        // CAS (Compare-And-Swap) loop to handle spurious failures and concurrent pushes.
        // - Expected: new_node->next (what we think the head is)
        // - Desired: new_node (what we want the head to become)
        while (!head.compare_exchange_weak(
            new_node->next,            // expected
            new_node,                  // desired
            std::memory_order_release, // memory order if SUCCESS
            std::memory_order_relaxed  // memory order if FAILURE
        )) {
            // If CAS fails (another thread pushed first, or spurious failure),
            // 'new_node->next' is AUTOMATICALLY updated to the latest actual head.
            // We just loop again to retry.
        }
    }

    bool pop(T& result) {
        // Use acquire to ensure we see the data written by the pushing thread
        Node* old_head = head.load(std::memory_order_acquire);

        // CAS loop to try swinging the head pointer to the next node
        while (old_head != nullptr &&
               !head.compare_exchange_weak(
                   old_head,                  // expected
                   old_head->next,            // desired
                   std::memory_order_acquire, // memory order if SUCCESS
                   std::memory_order_relaxed  // memory order if FAILURE
               )) {
            // If CAS fails, 'old_head' is automatically updated to the new head.
        }

        // If old_head is nullptr, the stack was empty
        if (old_head == nullptr) {
            return false; 
        }

        result = old_head->data;
        
        // WARNING: Deleting old_head directly in a lock-free structure can cause the ABA problem.
        // Robust implementations use memory reclamation techniques like Hazard Pointers.
        // We delete it here just to prevent a basic memory leak in this isolated example.
        delete old_head; 
        
        return true;
    }
};

// Thread worker function to test the stack
void worker(LockFreeStack<int>& stack, int start_val) {
    for (int i = 0; i < 5; ++i) {
        stack.push(start_val + i);
    }
}

int main() {
    LockFreeStack<int> lf_stack;

    // Create multiple threads pushing concurrently
    std::thread t1(worker, std::ref(lf_stack), 100);
    std::thread t2(worker, std::ref(lf_stack), 200);

    t1.join();
    t2.join();

    int val;
    // Pop all elements
    while (lf_stack.pop(val)) {
        std::cout << val << " ";
    }
    std::cout << "\n";

    return 0;
}