// ==============================================================================
// main.cpp — Entry point for coroutine-based stock poller
// ==============================================================================
//
// LEARNING OBJECTIVES (in order of execution):
// 1. How coro::scheduler is configured and what each option means
// 2. How to compose multiple concurrent tasks with coro::when_all
// 3. How coro::sync_wait bridges the sync/async boundary
// 4. How std::stop_source/stop_token enables graceful shutdown
// 5. How scheduler execution strategy affects latency and throughput
//
// EXECUTION FLOW:
//   main() → sync_wait(when_all([pollers...], [http_server]))
//     ├── poll_symbol("AAPL", scheduler, stop_token)
//     │   └── loop: simulate → yield_for(1s) → simulate → ...
//     ├── poll_symbol("GOOG", scheduler, stop_token)
//     │   └── loop: simulate → yield_for(1s) → simulate → ...
//     ├── ... (more symbols)
//     └── run_http_server(scheduler, "127.0.0.1", 8080)
//         └── loop: accept() → spawn_detached(handle_connection)
//
// All of the above runs on N threads (default: hardware_concurrency - 1).
// ==============================================================================

#include "poller.hpp"
#include "http_server.hpp"

#include <array>
#include <chrono>
#include <coro/coro.hpp>
#include <cstdio>
#include <csignal>
#include <cstdlib>

// ==============================================================================
// Global stop source for signal handler
// ==============================================================================
// LEARNING: In real production systems, you'd use a more sophisticated shutdown
// mechanism. This global is a simple pattern for educational purposes.
// Alternatives:
// - coro::event for async shutdown signaling
// - coro::condition_variable with stop_token for complex shutdown logic
// - Double-Ctrl+C forced exit pattern
// ==============================================================================
static std::stop_source g_stop_source;

static auto signal_handler(int sig) -> void
{
    fprintf(stderr, "\n[main] Received signal %d, requesting graceful shutdown...\n", sig);
    g_stop_source.request_stop();
}

// ==============================================================================
// MAIN — The bridge between synchronous and asynchronous worlds
// ==============================================================================
auto main() -> int
{
    // -------------------------------------------------------------------------
    // Install signal handlers for graceful shutdown
    // LEARNING: SIGINT (Ctrl+C) and SIGTERM (kill) should trigger clean shutdown.
    // In coroutine systems, you NEVER call exit() — it bypasses coroutine cleanup.
    // Instead, signal all coroutines to stop via stop_token, then wait for them.
    // -------------------------------------------------------------------------
    std::signal(SIGINT,  signal_handler);
    std::signal(SIGTERM, signal_handler);

    // -------------------------------------------------------------------------
    // Create the I/O Scheduler
    // LEARNING: The scheduler is the heart of a libcoro application.
    // It combines:
    //   - Thread pool (for parallel coroutine execution)
    //   - I/O event loop (epoll on Linux, kqueue on macOS)
    //   - Timer heap (for yield_for, schedule_after)
    //
    // Why scheduler instead of raw thread_pool?
    //   - thread_pool: CPU-bound parallel work only (no I/O, no timers)
    //   - scheduler: thread_pool + I/O + timers (everything we need)
    //
    // EXECUTION STRATEGY TRADE-OFFS:
    //   process_tasks_on_thread_pool (default):
    //     - Best for mixed I/O + CPU workloads
    //     - Events processed on I/O thread, tasks on pool threads
    //     - Lower latency for long-running tasks
    //
    //   process_tasks_inline:
    //     - Best for "thread per core" architecture
    //     - All work done on the I/O thread (no pool overhead)
    //     - Better cache locality, but blocks I/O on long tasks
    //     - Ideal for fast request/response patterns
    // -------------------------------------------------------------------------
    auto scheduler = coro::scheduler::make_unique(
        coro::scheduler::options{
            // Spawn a dedicated thread for the I/O event loop
            // Alternative: thread_strategy_t::manual → call process_events() yourself
            .thread_strategy = coro::scheduler::thread_strategy_t::spawn,

            // Configure the internal thread pool
            .pool = coro::thread_pool::options{
                // Use (cores - 1) threads — one core reserved for I/O thread
                // LEARNING: In HFT, you'd pin threads to specific cores with
                // pthread_setaffinity_np() via the on_thread_start_functor.
                .thread_count = std::max(2u, std::thread::hardware_concurrency() - 1),

                .on_thread_start_functor = [](std::size_t idx) {
                    // TODO: Set thread affinity to specific CPU core:
                    //   cpu_set_t cpuset;
                    //   CPU_ZERO(&cpuset);
                    //   CPU_SET(idx, &cpuset);
                    //   pthread_setaffinity_np(pthread_self(), sizeof(cpuset), &cpuset);
                    //
                    // TODO: Set thread priority to SCHED_FIFO for low latency:
                    //   struct sched_param param{.sched_priority = 80};
                    //   pthread_setschedparam(pthread_self(), SCHED_FIFO, &param);
                    //
                    // TODO: Set thread name for debugging:
                    //   pthread_setname_np("coro-worker-0");
                    fprintf(stderr, "[scheduler] Worker thread %zu started\n", idx);
                },
                .on_thread_stop_functor = [](std::size_t idx) {
                    fprintf(stderr, "[scheduler] Worker thread %zu stopped\n", idx);
                },
            },

            // Use thread pool for task execution (not inline on I/O thread)
            .execution_strategy = coro::scheduler::execution_strategy_t::process_tasks_on_thread_pool,
        }
    );

    fprintf(stderr, "[main] Scheduler created with %u worker threads\n",
            static_cast<unsigned>(std::max(2u, std::thread::hardware_concurrency() - 1)));

    // -------------------------------------------------------------------------
    // Define symbols to poll
    // LEARNING: Each symbol gets its own coroutine. This is MUCH more efficient
    // than one thread per symbol:
    //   - 1000 symbols × ~100 bytes frame = ~100 KB total coroutine memory
    //   - vs. 1000 threads × 8 MB stack = ~8 GB total thread memory
    //   - Plus: no thread context switches between polls
    // -------------------------------------------------------------------------
    const auto symbols = std::array{
        "AAPL",   // Apple
        "GOOG",   // Alphabet
        "MSFT",   // Microsoft
        "AMZN",   // Amazon
        "TSLA",   // Tesla
        "META",   // Meta Platforms
        "NVDA",   // NVIDIA
        "AMD",    // Advanced Micro Devices
        "NFLX",   // Netflix
        "SPY",    // S&P 500 ETF
    };

    // -------------------------------------------------------------------------
    // Collect all tasks for when_all
    // LEARNING: Tasks are LAZY. Creating a task does NOT start it.
    // Tasks only begin executing when they are co_awaited (directly or via
    // when_all). This is a critical difference from std::async (eager).
    //
    // WHY when_all instead of individual sync_wait?
    //   - sync_wait is BLOCKING — it halts the calling thread
    //   - when_all starts all tasks concurrently, then waits for ALL to finish
    //   - Without when_all, pollers would run one-at-a-time (sequential!)
    // -------------------------------------------------------------------------
    auto tasks = std::vector<coro::task<void>>{};
    tasks.reserve(symbols.size() + 1);  // +1 for HTTP server

    // -------------------------------------------------------------------------
    // Create poller tasks — one per symbol
    // LEARNING: Each call to make_poller_task() creates a coro::task<void>.
    // The task contains a coroutine frame with:
    //   - Copy of all function parameters (symbol, interval, etc.)
    //   - Local variables that survive across suspension points
    //   - Promise state for coroutine machinery
    //
    // NOTE: Different symbols get different polling intervals to demonstrate
    // that each coroutine has its own independent timer state.
    // -------------------------------------------------------------------------
    for (const auto* symbol : symbols)
    {
        // Vary intervals slightly to avoid thundering herd on shared resources
        // LEARNING: In HFT, you'd poll at exact intervals based on exchange
        // update frequency (e.g., 100ms for options, 1ms for Level 2 data)
        auto interval = std::chrono::milliseconds{500 + (symbol[0] % 5) * 100};

        tasks.emplace_back(
            make_poller_task(
                std::string{symbol},       // Copy symbol into coroutine frame
                scheduler,                 // Reference — scheduler must outlive tasks
                g_stop_source.get_token(), // Copy — each task gets its own token
                interval
            )
        );
    }

    // -------------------------------------------------------------------------
    // Create HTTP server task
    // LEARNING: The server runs as another task in when_all. It will:
    //   1. Bind to 127.0.0.1:8080
    //   2. Accept connections in a loop (async)
    //   3. Dispatch each connection to its own coroutine
    //   4. Stop when stop_token is signaled
    // -------------------------------------------------------------------------
    tasks.emplace_back(
        run_http_server(scheduler, "127.0.0.1", 8080, g_stop_source.get_token())
    );

    // -------------------------------------------------------------------------
    // Run everything: sync_wait + when_all
    // LEARNING: This is the key pattern for running concurrent coroutines:
    //
    //   sync_wait(when_all(tasks...))
    //       │          │
    //       │          └── Starts ALL tasks concurrently, returns when ALL finish
    //       └───────────── Blocks the main thread until everything completes
    //
    // WHAT HAPPENS INSIDE:
    // 1. when_all() creates a parent task that starts each child task
    // 2. Each child task is scheduled onto the scheduler's thread pool
    // 3. sync_wait() blocks main thread until the parent task completes
    // 4. The scheduler's event loop drives everything from its threads
    //
    // GRACEFUL SHUTDOWN FLOW:
    // 1. User presses Ctrl+C → signal_handler → g_stop_source.request_stop()
    // 2. All pollers check stop_tok.stop_requested() → exit loop → co_return
    // 3. HTTP server checks stop_tok.stop_requested() → exit accept loop
    // 4. when_all detects all tasks completed → parent task completes
    // 5. sync_wait unblocks → main() returns → clean exit
    // -------------------------------------------------------------------------
    fprintf(stderr, "[main] Starting %zu poller tasks + HTTP server on :8080\n",
            symbols.size());
    fprintf(stderr, "[main] Press Ctrl+C to initiate graceful shutdown\n");
    fprintf(stderr, "[main] Test HTTP: curl http://127.0.0.1:8080/\n\n");

    coro::sync_wait(coro::when_all(std::move(tasks)));

    fprintf(stderr, "\n[main] All tasks completed. Clean exit.\n");
    return 0;
}
