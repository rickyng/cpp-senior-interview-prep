# stock-poller-coro

A minimal, educational C++20/23 coroutine-based async stock/option data poller built on [libcoro](https://github.com/jbaldwin/libcoro).

**Purpose**: Self-learning template for modern C++ coroutines, concurrency primitives, and low-latency systems programming patterns.

---

## What Each File Teaches

### `main.cpp` — The Sync/Async Bridge
- How `coro::scheduler` is configured and what each option means
- How `coro::sync_wait()` bridges synchronous `main()` into the async coroutine world
- How `coro::when_all()` composes N concurrent tasks into one parent task
- How `std::stop_source`/`std::stop_token` enables cooperative graceful shutdown
- Why tasks are **lazy** (don't execute until awaited) vs `std::async` (eager)

### `poller.hpp` — Coroutine Fundamentals
- How `coro::task<void>` defines a coroutine with no return value
- How `co_await scheduler->yield_for()` suspends without blocking the thread
- **Critical**: Why `std::this_thread::sleep_for()` is FORBIDDEN in coroutines
- How `scheduler->schedule()` offloads to the thread pool
- Why coroutine parameters must be **by value** (not by reference) — [CP.51](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#Rcoro-capture)
- `quote_data` struct as a starting point for cache-aligned market data

### `http_server.hpp` — Async I/O and TCP Servers
- How `coro::net::tcp::server` accepts connections asynchronously
- How each connection gets its own coroutine via `spawn_detached()`
- How `read_some()` / `write_all()` work as awaitable async operations
- Minimal HTTP parsing and routing for learning purposes
- Placeholders for `/opportunities` JSON endpoint

### `CMakeLists.txt` — Modern Build Setup
- FetchContent for zero-hassle dependency management
- Low-latency compiler flags (`-O3 -march=native -flto`)
- Sanitizer integration (ASan, TSan, UBSan) for correctness validation
- Explanation of each CMake option

---

## Build & Run

### Prerequisites
- C++23 compiler (GCC 13+, Clang 17+, or Apple Clang 16+)
- CMake 3.22+
- Git (for FetchContent)
- pthreads (Linux/macOS)

### Build
```bash
cd stock-poller-coro
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j$(nproc 2>/dev/null || sysctl -n hw.ncpu)
```

### Run
```bash
./build/stock_poller
```

### Test the HTTP Server
```bash
# In another terminal:
curl http://127.0.0.1:8080/
curl http://127.0.0.1:8080/health
curl http://127.0.0.1:8080/opportunities
```

### Shutdown
Press `Ctrl+C` for graceful shutdown. All coroutines check `stop_token` and exit cleanly.

### Debug Build with Sanitizers
```bash
cmake -B build-debug -DCMAKE_BUILD_TYPE=Debug -DENABLE_SANITIZERS=ON
cmake --build build-debug -j$(nproc)
./build-debug/stock_poller
```

---

## Architecture

```
                        coro::scheduler
                    ┌──────────────────────┐
                    │  I/O Event Loop      │
                    │  (epoll / kqueue)    │
                    │                      │
                    │  Timer Heap          │──→ yield_for(1s) for each poller
                    │                      │
                    │  Thread Pool         │──→ N worker threads
                    │  (FIFO queue)        │
                    └──────┬───────────────┘
                           │
              ┌────────────┼────────────────┐
              │            │                │
         poll_symbol   poll_symbol    tcp::server
          ("AAPL")      ("GOOG")     (accept loop)
              │            │                │
         yield_for     yield_for     spawn_detached
          (1.2s)        (0.8s)      (per-connection)
```

**Key Insight**: 10 pollers + 1 server = 11 coroutines on `(cores-1)` threads. No thread per symbol. No blocking sleeps.

---

## Coroutine Concepts Learned

| Concept | Where Used | Key Insight |
|---------|-----------|-------------|
| `co_await` | `poller.hpp`, `http_server.hpp` | Suspends coroutine, frees thread for other work |
| `co_return` | All coroutines | Ends coroutine, signals awaiters |
| `coro::task<T>` | All functions | Lazy coroutine — doesn't start until awaited |
| `coro::scheduler` | `main.cpp` | Thread pool + I/O + timers in one executor |
| `sync_wait` | `main.cpp` | Bridges sync world (main) to async world (coroutines) |
| `when_all` | `main.cpp` | Runs N tasks concurrently, waits for ALL to finish |
| `yield_for` | `poller.hpp` | Async sleep — suspends coroutine, not thread |
| `spawn_detached` | `http_server.hpp` | Fire-and-forget task — scheduler owns lifetime |
| `stop_token` | `main.cpp`, `poller.hpp` | Cooperative cancellation — clean shutdown |

---

## Suggested Learning Extensions

### Easy (Beginner)
- [ ] Add more symbols to the polling list
- [ ] Change polling intervals and observe output timing
- [ ] Add a `/stats` endpoint that returns poller tick counts
- [ ] Experiment with `coro::when_any` (first-completed wins)

### Medium (Intermediate)
- [ ] Replace simulated data with real `coro::net::tcp::client` HTTP GET to a free API
- [ ] Add `coro::mutex` to protect a shared `std::map` of latest quotes
- [ ] Add `coro::event` to signal when all pollers have received their first tick
- [ ] Implement `coro::latch` to wait for initial data load before starting the server
- [ ] Use `coro::generator` to produce a stream of computed indicators (SMA, VWAP)

### Advanced (Senior/Staff)
- [ ] Replace `std::map` with a lock-free concurrent hash map (Folly's `F14` or custom)
- [ ] Add `std::pmr::memory_resource` for pre-allocated coroutine frames (arena allocator)
- [ ] Integrate [simdjson](https://github.com/simdjson/simdjson) for zero-copy JSON parsing
- [ ] Add `alignas(64)` cache padding to `quote_data` to prevent false sharing
- [ ] Implement NUMA-aware thread pinning in `on_thread_start_functor`
- [ ] Add `coro::ring_buffer` as an SPSC queue between pollers and strategy engine
- [ ] Measure tail latency (p99, p999) with `std::chrono` and optimize hot paths
- [ ] Experiment with `io_uring` (Linux) for zero-syscall I/O
- [ ] Add coroutine-based backpressure with `coro::semaphore` on the HTTP server

---

## License

Educational use only. Built on [libcoro](https://github.com/jbaldwin/libcoro) (Apache 2.0).


  ---                                                                                                                                                                                            
  Project Structure                                                                                                                                                                              
                                                                                                                                                    
  stock-poller-coro/                                                                                                                                                                             
  ├── CMakeLists.txt    (124 lines) — Modern CMake, FetchContent, low-latency flags                                                                                                              
  ├── main.cpp          (238 lines) — Scheduler setup, when_all, sync_wait, stop_token                                                                                                           
  ├── poller.hpp        (223 lines) — Coroutine poller: yield_for, co_await, quote_data
  ├── http_server.hpp   (287 lines) — TCP server: accept loop, per-connection coroutine                                                                                                          
  └── README.md         (154 lines) — Learning guide with build instructions                                                                                                                     
                                                                                                                                                                                                 
  What Was Built & Verified                                                                                                                                                                      
                                                                                                                                                                                                 
  - Zero-warning build with Apple Clang 17, C++23, Release mode                                                                                                                                  
  - 10 concurrent poller coroutines (AAPL, GOOG, MSFT, etc.) on 7 worker threads
  - HTTP server on 127.0.0.1:8080 with /, /health, /opportunities endpoints
  - Graceful shutdown via std::stop_source/std::stop_token + SIGINT handler

  Key libcoro API Usage

  ┌──────────────────┬────────────────────────────────────────────────────────────────┐
  │     Pattern      │                            API Used                            │
  ├──────────────────┼────────────────────────────────────────────────────────────────┤
  │ Executor         │ coro::scheduler::make_unique() with thread pool + I/O + timers │
  ├──────────────────┼────────────────────────────────────────────────────────────────┤
  │ Task composition │ coro::when_all(tasks...) + coro::sync_wait()                   │
  ├──────────────────┼────────────────────────────────────────────────────────────────┤
  │ Async sleep      │ scheduler->yield_for(interval) (not sleep_for!)                │
  ├──────────────────┼────────────────────────────────────────────────────────────────┤
  │ TCP accept loop  │ coro::net::tcp::server + co_await server.accept()              │
  ├──────────────────┼────────────────────────────────────────────────────────────────┤
  │ Fire-and-forget  │ scheduler->spawn_detached(handle_connection(...))              │
  ├──────────────────┼────────────────────────────────────────────────────────────────┤
  │ Cancellation     │ std::stop_source → std::stop_token per coroutine               │
  └──────────────────┴────────────────────────────────────────────────────────────────┘

  Build & Run

  cd stock-poller-coro
  cmake -B build -DCMAKE_BUILD_TYPE=Release
  cmake --build build -j$(nproc)
  ./build/stock_poller          # Ctrl+C to stop
  curl http://127.0.0.1:8080/   # test in another terminal
