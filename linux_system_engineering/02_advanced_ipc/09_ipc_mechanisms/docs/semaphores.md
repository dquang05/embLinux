# Semaphores

System V Semaphores are an IPC mechanism used for synchronization rather than data transfer. They are kernel-maintained integers with values >= 0.

## 1. System V Semaphores Overview
- Semaphores are allocated in **sets** (groups). You can operate on one or multiple semaphores atomically.
- **Operations Supported**:
  - Add to a value (releases a resource).
  - Subtract from a value (reserves a resource). If the result would be < 0, the process blocks.
  - Wait for the value to equal 0. Blocks if the value != 0.
- **Associated Data Structure**: `semid_ds` (contains timestamps, permissions, number of semaphores).
- **System Calls**:
  - `semget()`: Creates a new set or opens an existing one.
  - `semctl()`: Control operations (`SETVAL`, `GETVAL`, `IPC_RMID`, etc.). Requires a custom `union semun` defined by the user for certain calls.
  - `semop()`: Performs one or more operations atomically using an array of `struct sembuf`.

## 2. Semaphore Initialization Race Condition
System V Semaphores separate creation (`semget`) and initialization (`semctl`). If multiple peer processes attempt to create and initialize the semaphore concurrently, a race condition occurs.
- **Solution**: The process that successfully creates the semaphore (via `IPC_CREAT | IPC_EXCL`) initializes it. Other processes wait for `sem_otime` (time of last `semop`) to become non-zero by looping and calling `semctl` with `IPC_STAT` before proceeding.

## 3. SEM_UNDO and Process Termination
If a process adjusts a semaphore (e.g., locks it) and unexpectedly terminates, the resource remains locked forever.
- Using the `SEM_UNDO` flag with `semop()` tells the kernel to track the process's adjustments (in a `semadj` variable) and automatically revert them upon termination.

## 4. Binary Semaphores (Mutexes)
A common protocol built on System V Semaphores.
- Value `1` = Available/Free.
- Value `0` = Reserved/In-use.
- **Reserve (P/Wait/Down)**: `semop` with `-1`.
- **Release (V/Post/Up)**: `semop` with `+1`.

## 5. POSIX Semaphores
POSIX Semaphores provide a simpler API compared to System V Semaphores and come in two types: Named (accessible by unrelated processes via a name starting with `/`) and Unnamed (residing in shared memory for processes or threads).
- **Key Differences from System V**:
  - **Single Operations**: `sem_wait()` and `sem_post()` operate on just one semaphore at a time, incrementing or decrementing by exactly 1.
  - **Simpler Initialization**: Named semaphores are created and initialized atomically using `sem_open()`, avoiding the race condition inherent in System V Semaphores.
  - **Reference Counted**: Safe deletion via `sem_close()` and `sem_unlink()`.
  - No wait-for-zero operation.
- **Named Semaphores API**:
  - `sem_open()`: Creates/opens a named semaphore and initializes its value.
  - `sem_wait()`: Decrements the semaphore. Blocks if value is 0.
  - `sem_post()`: Increments the semaphore.
  - `sem_close()`: Closes the semaphore for the calling process.
  - `sem_unlink()`: Removes the semaphore name and marks it for deletion.
- **Linux Specifics**:
  - Named semaphores are created as small POSIX shared memory objects in `/dev/shm` (names prefixed with `sem.`).
