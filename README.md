# Concurrent-Message-Pipeline-Architecture
A high-performance producer-consumer message pipeline in C, evolving from a POSIX thread-based queue (mutexes/condition variables) to a dual-process architecture with System V shared memory and semaphore synchronization.
# Concurrent Message Pipeline

A high-performance, multithreaded message queue system in C, built around the classic producer-consumer problem. The project evolves in two stages: a single-process design using POSIX threads, mutexes, and condition variables, followed by a distributed dual-process architecture using System V shared memory and semaphore-based IPC.

## Overview

This project demonstrates safe, high-throughput data flow between concurrent producers and a consumer, first within a single process (threads) and then across process boundaries (shared memory + semaphores) — with a focus on correctness under concurrent load, not just a working happy path.

**Version 1 — Multithreaded Queue**
Multiple producer threads push messages into a shared, fixed-size circular buffer; a single consumer thread drains it. Synchronization is handled with `pthread_mutex_t` for mutual exclusion and two `pthread_cond_t` condition variables (`not_full`, `not_empty`) to avoid busy-waiting.

**Version 2 — Dual-Process Architecture**
The same producer-consumer logic is re-implemented across two separate OS processes. Since POSIX mutexes and condition variables don't work across process boundaries, synchronization is rebuilt using System V semaphores (`semget`/`semop`), and the shared ring buffer lives in a System V shared memory segment (`shmget`/`shmat`), enabling zero-copy data transfer between processes.

## Features

- Fixed-size circular (ring) buffer for bounded, memory-efficient queuing
- Multiple producers, single consumer, fully synchronized
- Zero busy-waiting — producers and consumer block efficiently via condition variables / semaphores instead of polling
- Race-condition-free access to shared state (`head`, `tail`, `count`), verified through targeted debugging and testing
- Deadlock-free design with documented lock-ordering and signaling discipline
- Cross-process IPC using shared memory and semaphores, avoiding kernel-mediated data copies
- Stress-tested with sequence-number tracking to verify zero message loss, no duplicates, and correct per-producer ordering

## Architecture

```
Version 1 (threads)
 Producer 1 ─┐
 Producer 2 ─┼──► [ Ring Buffer ] ──► Consumer
 Producer 3 ─┘     mutex + cond vars

Version 2 (processes)
 Producer Process ──► [ Shared Memory Ring Buffer ] ──► Consumer Process
                        semaphores: mutex / empty / full
```

## Project Structure

```
.
├── src/
│   ├── queue.c / queue.h          # Ring buffer + pthread synchronization (Version 1)
│   ├── producer.c                 # Multithreaded producer/consumer demo
│   ├── ipc_common.h               # Shared struct definitions (Version 2)
│   ├── sem_helpers.h              # System V semaphore wait/post helpers
│   ├── producer_process.c         # Producer process (Version 2)
│   └── consumer_process.c         # Consumer process (Version 2)
├── tests/
│   └── stress_test.c              # Multi-producer stress test with sequence verification
└── README.md
```

## Building

```bash
# Version 1 — multithreaded
gcc -g -Wall -Wextra -pthread src/queue.c src/producer.c -o pipeline_threads

# Version 2 — dual-process
gcc -g -Wall -Wextra src/producer_process.c -o producer
gcc -g -Wall -Wextra src/consumer_process.c -o consumer
```

## Running

**Threaded version:**
```bash
./pipeline_threads
```

**Dual-process version:**
```bash
touch /tmp/pipeline_keyfile
./producer &
./consumer
```

## Testing

The stress test spawns multiple producer threads sending a large volume of sequence-numbered messages under randomized delays, then verifies:

- **Zero message loss** — every sequence number sent was received
- **No duplicates** — no message delivered more than once
- **Ordering** — per-producer message order is preserved
- **Data integrity** — payload contents match expected values
- **No deadlocks** — all threads complete within a bounded time

```bash
gcc -g -Wall -Wextra -pthread tests/stress_test.c src/queue.c -o stress_test
./stress_test
```

For race-condition verification beyond observed test outcomes, the codebase has also been run under ThreadSanitizer and Valgrind's Helgrind:

```bash
gcc -g -fsanitize=thread -pthread src/queue.c src/producer.c -o pipeline_tsan
./pipeline_tsan
```

## Debugging Tools Used

- `gdb` — inspecting thread state and diagnosing hangs (`thread apply all bt`)
- AddressSanitizer / ThreadSanitizer — memory errors and data races
- Valgrind (Helgrind) — synchronization and lock-order analysis
- `strace` — verifying shared memory and semaphore syscalls
- `ipcs` / `ipcrm` — inspecting and cleaning up System V IPC resources

## Key Concepts Demonstrated

- POSIX threads: `pthread_create`, `pthread_join`, `pthread_mutex_t`, `pthread_cond_t`
- The producer-consumer problem and bounded-buffer synchronization
- Correct condition-variable usage (`while` vs `if`, avoiding lost wakeups and spurious wakeups)
- Race conditions, critical sections, and lock scoping
- Deadlocks, starvation, and livelock — causes and prevention
- System V IPC: `shmget`, `shmat`, `shmdt`, `shmctl`, `semget`, `semop`, `semctl`
- Performance considerations: synchronization overhead, context switches, cache effects, and lock contention

## Author

**Kishan Soya**
