# Higher-Level Concurrency Patterns

After learning synchronization primitives such as mutexes, condition variables, futures, and timeouts, it is useful to explore higher-level concurrency patterns that reduce the need for explicit synchronization.

Two common approaches are:

1. Functional Programming (FP) with Futures
2. Actor Model (Message Passing)

---

# 1. Functional Programming (FP) with Futures

## Concept

Functional Programming emphasizes **pure functions**, where the result depends solely on the input parameters.

By avoiding shared mutable state, many concurrency issues such as race conditions are eliminated, reducing the need for manual mutex protection.

---

## Implementation Patterns

### `std::async`

Used to launch tasks asynchronously.

```cpp
auto future = std::async(function, args...);
```

### `std::future`

Used to retrieve results from asynchronous tasks.

```cpp
auto result = future.get();
```

The calling thread blocks until the result becomes available.

---

## Case Study: Parallel Quicksort

Instead of sorting in place, the functional approach creates new lists and sorts sublists independently.

### Mechanism

1. Split the input into smaller segments.
2. Launch recursive sorting tasks using `std::async`.
3. Use `future.get()` to retrieve the sorted segments.
4. Combine the results into a final sorted list.

This approach naturally exploits hardware concurrency while avoiding shared mutable data.

---

## Caution

Excessive use of `std::async` in recursive algorithms may lead to **oversubscription**, where more threads are created than the system can efficiently execute.

Some implementations may mitigate this by executing tasks synchronously when necessary.

---

# 2. Actor Model (Message Passing)

## Concept

The Actor Model treats threads as independent entities called **actors**.

Actors do not share state directly. Instead, they communicate exclusively through message queues.

This approach simplifies concurrent logic by modeling each actor as a **Finite State Machine (FSM)**.

---

## Implementation Structure

### State Representation

Each state is typically implemented as a member function.

### State Machine

The actor stores a pointer to its current state function.

Example:

```cpp
void (atm::*state)();
```

### Main Loop

The actor repeatedly executes the function corresponding to its current state.

### Message Handling

Each state specifies which messages it can process.

```cpp
incoming.wait()
    .handle<T1>(
        [&](T1 const& msg)
        {
            /* Process message and transition state */
        })
    .handle<T2>(
        [&](T2 const& msg)
        {
            /* Process message */
        });
```

Message handlers define the behavior of the current state and may trigger state transitions.

---

# 3. Comparison Summary

| Feature           | Functional Programming                   | Actor Model                                |
| ----------------- | ---------------------------------------- | ------------------------------------------ |
| Data Handling     | Immutable; data flows through parameters | Isolated; state changes via messages       |
| Primary Tool      | `std::future`, `std::async`              | Message Queues, State Machines             |
| Design Focus      | Mathematical composition of functions    | Independent behavior of independent actors |
| Concurrency Logic | Implicitly handled by task decomposition | Explicitly handled by message routing      |

---

# 4. Cautions and Discipline

## Shared Address Space

Although these patterns discourage shared mutable state, C++ threads still share the same process address space.

Correct usage relies on developer discipline rather than language enforcement.

---

## Task Decomposition

Both approaches require careful system design.

The focus should be on:

* Dividing work into independent functional units.
* Separating behavior into independent actors.

Rather than simply adding low-level synchronization primitives to existing code.

---

## Scaling Considerations

Overusing `std::async` in recursive or highly parallel algorithms can create excessive numbers of threads.

Depending on the implementation, some tasks may be executed synchronously to avoid exceeding practical thread limits.
