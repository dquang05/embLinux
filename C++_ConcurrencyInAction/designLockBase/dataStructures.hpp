#ifndef CONCURRENT_CONTAINERS_HPP
#define CONCURRENT_CONTAINERS_HPP

#include <exception>
#include <stack>
#include <queue>
#include <list>
#include <vector>
#include <map>
#include <algorithm>
#include <mutex>
#include <shared_mutex>
#include <memory>
#include <condition_variable>
#include <functional>

// ============================================================================
// CLASS 1: threadsafe_stack 
// ----------------------------------------------------------------------------
// TYPE: Coarse-grained (Single Lock)
// DESCRIPTION: 
// A basic wrapper around std::stack<>. Uses one global mutex for all operations.
// ============================================================================

struct empty_stack : std::exception
{
    const char* what() const throw() override {
        return "Stack is empty!";
    }
};

template<typename T>
class threadsafe_stack
{
private:
    std::stack<T> data;
    mutable std::mutex m;

public:
    threadsafe_stack() {}

    threadsafe_stack(const threadsafe_stack& other)
    {
        std::lock_guard<std::mutex> lock(other.m);
        data = other.data;
    }

    threadsafe_stack& operator=(const threadsafe_stack&) = delete;

    void push(T new_value)
    {
        std::lock_guard<std::mutex> lock(m);
        data.push(std::move(new_value)); 
    }

    std::shared_ptr<T> pop()
    {
        std::lock_guard<std::mutex> lock(m);
        if(data.empty()) throw empty_stack(); 
        
        // Allocate memory before mutating container for exception safety
        std::shared_ptr<T> const res(std::make_shared<T>(std::move(data.top()))); 
        data.pop(); 
        return res;
    }

    void pop(T& value)
    {
        std::lock_guard<std::mutex> lock(m);
        if(data.empty()) throw empty_stack();
        value = std::move(data.top()); 
        data.pop(); 
    }

    bool empty() const
    {
        std::lock_guard<std::mutex> lock(m);
        return data.empty();
    }
};


// ============================================================================
// CLASS 2: threadsafe_queue_single_mutex 
// ----------------------------------------------------------------------------
// TYPE: Coarse-grained + Condition Variable
// DESCRIPTION: 
// A thread-safe queue wrapped around std::queue<std::shared_ptr<T>>.
// Supports event-driven blocking (wait_and_pop) instead of busy-polling.
// ============================================================================

template<typename T>
class threadsafe_queue_single_mutex
{
private:
    mutable std::mutex mut;
    std::queue<std::shared_ptr<T>> data_queue;
    std::condition_variable data_cond;

public:
    threadsafe_queue_single_mutex() {}

    void push(T new_value)
    {
        // Allocation is done outside the lock to minimize time holding the mutex
        std::shared_ptr<T> data(std::make_shared<T>(std::move(new_value))); 
        
        std::lock_guard<std::mutex> lk(mut);
        data_queue.push(data);
        data_cond.notify_one();
    }

    void wait_and_pop(T& value)
    {
        std::unique_lock<std::mutex> lk(mut);
        data_cond.wait(lk, [this]{ return !data_queue.empty(); });
        value = std::move(*data_queue.front());
        data_queue.pop();
    }

    std::shared_ptr<T> wait_and_pop()
    {
        std::unique_lock<std::mutex> lk(mut);
        data_cond.wait(lk, [this]{ return !data_queue.empty(); });
        std::shared_ptr<T> res = data_queue.front();
        data_queue.pop();
        return res;
    }

    bool try_pop(T& value)
    {
        std::lock_guard<std::mutex> lk(mut);
        if(data_queue.empty()) return false;
        value = std::move(*data_queue.front());
        data_queue.pop();
        return true;
    }

    std::shared_ptr<T> try_pop()
    {
        std::lock_guard<std::mutex> lk(mut);
        if(data_queue.empty()) return std::shared_ptr<T>();
        std::shared_ptr<T> res = data_queue.front();
        data_queue.pop();
        return res;
    }

    bool empty() const
    {
        std::lock_guard<std::mutex> lk(mut);
        return data_queue.empty();
    }
};


// ============================================================================
// CLASS 3: threadsafe_queue_fine_grained 
// ----------------------------------------------------------------------------
// TYPE: Fine-grained (Two Mutexes + Dummy Node)
// DESCRIPTION: 
// A highly concurrent queue built from scratch using a Singly Linked List.
// Employs a permanent "Dummy Node" to isolate 'head' and 'tail' mutations, 
// allowing simultaneous push and pop operations.
// ============================================================================

template<typename T>
class threadsafe_queue_fine_grained
{
private:
    struct node
    {
        std::shared_ptr<T> data;
        std::unique_ptr<node> next;
    };

    std::mutex head_mutex;
    std::unique_ptr<node> head;
    std::mutex tail_mutex;
    node* tail;
    std::condition_variable data_cond;

    node* get_tail()
    {
        std::lock_guard<std::mutex> tail_lock(tail_mutex);
        return tail;
    }

    std::unique_ptr<node> pop_head() 
    {
        std::unique_ptr<node> old_head = std::move(head);
        head = std::move(old_head->next);
        return old_head;
    }

    std::unique_lock<std::mutex> wait_for_data() 
    {
        std::unique_lock<std::mutex> head_lock(head_mutex);
        // CRITICAL: get_tail() must be evaluated inside the protected head region
        data_cond.wait(head_lock, [&]{ return head.get() != get_tail(); });
        return std::move(head_lock); 
    }

    std::unique_ptr<node> wait_pop_head()
    {
        std::unique_lock<std::mutex> head_lock(wait_for_data()); 
        return pop_head();
    }

    std::unique_ptr<node> wait_pop_head(T& value)
    {
        std::unique_lock<std::mutex> head_lock(wait_for_data()); 
        // Data assignment happens before unlinking the node for exception safety
        value = std::move(*head->data);
        return pop_head();
    }

    std::unique_ptr<node> try_pop_head()
    {
        std::lock_guard<std::mutex> head_lock(head_mutex);
        if(head.get() == get_tail())
        {
            return std::unique_ptr<node>();
        }
        return pop_head();
    }

    std::unique_ptr<node> try_pop_head(T& value)
    {
        std::lock_guard<std::mutex> head_lock(head_mutex);
        if(head.get() == get_tail())
        {
            return std::unique_ptr<node>();
        }
        value = std::move(*head->data);
        return pop_head();
    }

public:
    // Initialize list with a single dummy node where head == tail
    threadsafe_queue_fine_grained() :
        head(new node), tail(head.get()) {}

    threadsafe_queue_fine_grained(const threadsafe_queue_fine_grained& other) = delete;
    threadsafe_queue_fine_grained& operator=(const threadsafe_queue_fine_grained& other) = delete;

    void push(T new_value)
    {
        std::shared_ptr<T> new_data(std::make_shared<T>(std::move(new_value)));
        std::unique_ptr<node> p(new node);
        
        {
            std::lock_guard<std::mutex> tail_lock(tail_mutex);
            tail->data = new_data;
            node* const new_tail = p.get();
            tail->next = std::move(p);
            tail = new_tail;
        }
        data_cond.notify_one();
    }

    std::shared_ptr<T> wait_and_pop()
    {
        std::unique_ptr<node> const old_head = wait_pop_head();
        return old_head->data;
    }

    void wait_and_pop(T& value)
    {
        std::unique_ptr<node> const old_head = wait_pop_head(value);
    }

    std::shared_ptr<T> try_pop()
    {
        std::unique_ptr<node> old_head = try_pop_head();
        return old_head ? old_head->data : std::shared_ptr<T>();
    }

    bool try_pop(T& value)
    {
        std::unique_ptr<node> const old_head = try_pop_head(value);
        return old_head != nullptr;
    }

    bool empty()
    {
        std::lock_guard<std::mutex> head_lock(head_mutex);
        return (head.get() == get_tail());
    }
};


// ============================================================================
// CLASS 4: threadsafe_lookup_table 
// ----------------------------------------------------------------------------
// TYPE: Fine-grained Hash Map (Lock-per-Bucket + Shared Locks)
// DESCRIPTION: 
// A concurrent map partitioned into dynamic discrete buckets. 
// Leverages std::shared_mutex (C++17) to facilitate non-exclusive concurrent 
// reader threads alongside strict single-writer locks.
// ============================================================================

template<typename Key, typename Value, typename Hash = std::hash<Key>>
class threadsafe_lookup_table
{
private:
    class bucket_type
    {
    private:
        typedef std::pair<Key, Value> bucket_value;
        typedef std::list<bucket_value> bucket_data;
        typedef typename bucket_data::iterator bucket_iterator;

        bucket_data data;
        mutable std::shared_mutex mutex; 

        bucket_iterator find_entry_for(Key const& key)
        {
            return std::find_if(data.begin(), data.end(),
                [&](bucket_value const& item) { return item.first == key; });
        }

        typename bucket_data::const_iterator find_entry_for(Key const& key) const
        {
            return std::find_if(data.begin(), data.end(),
                [&](bucket_value const& item) { return item.first == key; });
        }

    public:
        bucket_type() {}

        Value value_for(Key const& key, Value const& default_value) const
        {
            std::shared_lock<std::shared_mutex> lock(mutex); 
            auto const found_entry = find_entry_for(key);
            return (found_entry == data.end()) ? default_value : found_entry->second;
        }

        void add_or_update_mapping(Key const& key, Value const& value)
        {
            std::unique_lock<std::shared_mutex> lock(mutex); 
            auto found_entry = find_entry_for(key);
            if (found_entry == data.end())
            {
                data.push_back(bucket_value(key, value));
            }
            else
            {
                found_entry->second = value;
            }
        }

        void remove_mapping(Key const& key)
        {
            std::unique_lock<std::shared_mutex> lock(mutex); 
            auto found_entry = find_entry_for(key);
            if (found_entry != data.end())
            {
                data.erase(found_entry);
            }
        }

        friend class threadsafe_lookup_table;
    };

    std::vector<std::unique_ptr<bucket_type>> buckets; 
    Hash hasher;

    bucket_type& get_bucket(Key const& key) const 
    {
        // Safe to call without locks because buckets vector size is read-only post-construction
        std::size_t const bucket_index = hasher(key) % buckets.size();
        return *buckets[bucket_index];
    }

public:
    typedef Key key_type;
    typedef Value mapped_type;
    typedef Hash hash_type;

    threadsafe_lookup_table(unsigned num_buckets = 19, Hash const& hasher_ = Hash()) :
        buckets(num_buckets), hasher(hasher_)
    {
        for (unsigned i = 0; i < num_buckets; ++i)
        {
            buckets[i].reset(new bucket_type);
        }
    }

    threadsafe_lookup_table(threadsafe_lookup_table const& other) = delete;
    threadsafe_lookup_table& operator=(threadsafe_lookup_table const& other) = delete;

    Value value_for(Key const& key, Value const& default_value = Value()) const
    {
        return get_bucket(key).value_for(key, default_value); 
    }

    void add_or_update_mapping(Key const& key, Value const& value)
    {
        get_bucket(key).add_or_update_mapping(key, value); 
    }

    void remove_mapping(Key const& key)
    {
        get_bucket(key).remove_mapping(key); 
    }

    std::map<Key, Value> get_map() const
    {
        std::vector<std::unique_lock<std::shared_mutex>> locks;
        locks.reserve(buckets.size());
        
        // CRITICAL: To prevent cross-thread deadlocks, locks must be acquired 
        // in strict ascending index order.
        for (unsigned i = 0; i < buckets.size(); ++i)
        {
            locks.push_back(std::unique_lock<std::shared_mutex>(buckets[i]->mutex));
        }

        std::map<Key, Value> res;
        for (unsigned i = 0; i < buckets.size(); ++i)
        {
            for (auto it = buckets[i]->data.begin(); it != buckets[i]->data.end(); ++it)
            {
                res.insert(*it);
            }
        }
        return res;
    }
};


// ============================================================================
// CLASS 5: threadsafe_list 
// ----------------------------------------------------------------------------
// TYPE: Fine-grained List (Lock-per-Node / Hand-over-hand Locking)
// DESCRIPTION: 
// A singly linked list where every node carries its own independent mutex.
// Iteration traverses via functional callbacks rather than raw STL iterators.
// ============================================================================

template<typename T>
class threadsafe_list
{
private:
    struct node 
    {
        std::mutex m; 
        std::shared_ptr<T> data;
        std::unique_ptr<node> next;

        node() : next() {} // Head dummy constructor
        node(T const& value) : data(std::make_shared<T>(value)) {}
    };

    node head; // Dummy root node anchoring the beginning of the chain

public:
    threadsafe_list() {}

    ~threadsafe_list()
    {
        // Wipe all links safely during destruction sequence
        remove_if([](T const&) { return true; });
    }

    threadsafe_list(threadsafe_list const& other) = delete;
    threadsafe_list& operator=(threadsafe_list const& other) = delete;

    void push_front(T const& value)
    {
        std::unique_ptr<node> new_node(new node(value)); 
        std::lock_guard<std::mutex> lk(head.m);
        new_node->next = std::move(head.next); 
        head.next = std::move(new_node); 
    }

    template<typename Function>
    void for_each(Function f) 
    {
        node* current = &head;
        std::unique_lock<std::mutex> lk(head.m); 
        
        while (node* const next = current->next.get()) 
        {
            // HAND-OVER-HAND STEP: Lock the forward node prior to dropping the current lock
            std::unique_lock<std::mutex> next_lk(next->m); 
            lk.unlock(); 
            
            f(*next->data); 
            
            current = next;
            lk = std::move(next_lk); 
        }
    }

    template<typename Predicate>
    std::shared_ptr<T> find_first_if(Predicate p) 
    {
        node* current = &head;
        std::unique_lock<std::mutex> lk(head.m);
        
        while (node* const next = current->next.get())
        {
            std::unique_lock<std::mutex> next_lk(next->m);
            lk.unlock();
            
            if (p(*next->data)) 
            {
                return next->data; // Clean early return, unique_lock releases instantly
            }
            current = next;
            lk = std::move(next_lk);
        }
        return std::shared_ptr<T>();
    }

    template<typename Predicate>
    void remove_if(Predicate p) 
    {
        node* current = &head;
        std::unique_lock<std::mutex> lk(head.m);
        
        while (node* const next = current->next.get())
        {
            std::unique_lock<std::mutex> next_lk(next->m);
            
            if (p(*next->data)) 
            {
                // Decouple node while holding both active bounding locks
                std::unique_ptr<node> old_next = std::move(current->next);
                current->next = std::move(next->next); 
                
                next_lk.unlock();
                // old_next falls out of scope here; memory cleanup happens safely outside lock chain
            } 
            else
            {
                lk.unlock(); 
                current = next;
                lk = std::move(next_lk);
            }
        }
    }
};

#endif