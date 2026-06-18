# Introduction

## Moving Beyond Basic Tools

Designing concurrent code is not merely about using basic synchronization primitives such as mutexes, condition variables, or individual thread-safe data structures. It requires a broader perspective to build large-scale architectures and perform useful work across the entire application.

## Design Considerations

In addition to traditional software design principles such as encapsulation, cohesion, and coupling, concurrent programming must address several additional challenges:

- Which data should be shared, and which data should remain independent?
- How should access to shared data be synchronized to prevent data races?
- Which threads must wait for other threads to complete intermediate steps?
- How many threads should be used to achieve optimal performance on the available hardware?

## Objective

Choosing the appropriate thread organization and structure has a decisive impact on both system performance and code clarity and maintainability.

# Work-Sharing Techniques Between Threads

This section focuses on different approaches to dividing work among threads in order to fully utilize multicore processors.

## Dividing Data Before Processing Begins

- Suitable for simple data-processing algorithms such as `std::for_each`.
- The data set is divided into fixed-size partitions before execution begins.
- Each thread works independently on its assigned portion of the data.
- Results are combined in a final reduction step.

## Dividing Data Recursively

- Applicable to algorithms where the structure of the data becomes apparent only during processing, such as Quicksort.
- Instead of creating a new thread for every recursive branch, which can lead to excessive context-switching overhead, the number of threads is limited.
- A shared work pool is used to distribute tasks dynamically, typically implemented with a Thread Pool and a Thread-Safe Stack.

## Dividing Work by Task Type

- Threads are assigned specialized responsibilities, following the principle of Separation of Concerns.
- This approach helps maintain responsive user interfaces and timely network processing.

### Pipeline Architecture

- Work can also be organized as a pipeline, where each thread is responsible for a specific stage in a sequence of processing steps.
- The output of one stage becomes the input of the next stage.
- This allows data to flow continuously through the system, producing results more smoothly and consistently.