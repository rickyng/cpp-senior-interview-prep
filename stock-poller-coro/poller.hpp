#pragma once
// ==============================================================================
// poller.hpp — Coroutine-based market data poller
// ==============================================================================
//
// LEARNING OBJECTIVES:
// 1. How coro::task<void> defines an async coroutine that completes with no value
// 2. How co_await scheduler->yield_for() suspends without blocking the thread
// 3. Why std::this_thread::sleep_for() is FORBIDDEN in coroutine context
// 4. How scheduler->schedule() offloads execution to the thread pool
// 5. Graceful shutdown via std::stop_token cooperative cancellation
//
// ARCHITECTURE NOTES:
// - Each symbol gets its own coroutine (cheap: ~100 bytes frame on heap)
// - All pollers share the same scheduler (no per-poller thread needed)
// - The scheduler's internal thread pool distributes work across N threads
// - yield_for() uses kernel epoll/kqueue timers — zero CPU while waiting
//
// FUTURE LOW-LATENCY IMPROVEMENTS (marked with TODO):
// - Replace simulated data with real async HTTP GET via coro::net::tcp::client
// - Add simdjson for zero-allocation JSON parsing of market data
// - Replace std::string symbol with std::string_view or interned symbol IDs
// - Use lock-free SPSC ring buffer to push data to strategy engine
// - Add cache-aligned tick data structs (alignas(64))
// - Use std::pmr::memory_resource for pre-allocated coroutine frames
// ==============================================================================

#include <chrono>
#include <coro/coro.hpp>
#include <cstdio>       // fprintf for lock-free-ish logging
#include <functional>   // std::hash
#include <string>
#include <string_view>

// ==============================================================================
// Simulated Quote Structure
// ==============================================================================
// LEARNING: In real HFT systems, this would be a POD type with alignas(64)
// to prevent false sharing between CPU cores. For now, we use a simple struct
// that mimics what a real market data feed would provide.
// ==============================================================================
struct quote_data
{
    std::string symbol{};   // TODO: replace with interned uint32_t symbol_id
    double      bid{0.0};
    double      ask{0.0};
    uint64_t    timestamp_ns{0};  // nanoseconds since epoch
    uint32_t    bid_size{0};
    uint32_t    ask_size{0};
};

// ==============================================================================
// poll_symbol() — Core polling coroutine
// ==============================================================================
//
// LEARNING: This is the heart of the async poller. Key concepts:
//
// 1. RETURN TYPE: coro::task<void>
//    - A coroutine that completes without returning a value
//    - The promise_type inside coro::task manages the coroutine frame lifetime
//    - Tasks are LAZY — they don't start executing until co_awaited
//
// 2. PARAMETERS BY VALUE (not by reference):
//    - CRITICAL RULE from C++ Core Guidelines CP.51:
//      Do NOT use capturing lambdas that are coroutines
//    - Pass all data by value into coroutine function arguments
//    - Lambda captures are destroyed at the FIRST suspension point
//    - This is why symbol is std::string (copies in), not const std::string&
//
// 3. COOPERATIVE CANCELLATION via std::stop_token:
//    - C++20's jthread/stop_source/stop_token pattern
//    - The scheduler can request_stop() to signal all pollers
//    - Each poller checks token.stop_requested() in its loop
//    - No thread interruption or forced termination — clean shutdown
//
// 4. WHY NOT coro::thread_pool::yield_for()?
//    - coro::thread_pool does NOT support timed suspension
//    - Only coro::scheduler provides yield_for() / schedule_after()
//    - This is because thread_pool is for CPU-bound work only
//    - scheduler wraps thread_pool AND adds I/O + timer backends
//
// ==============================================================================
auto poll_symbol(
    std::string                          symbol,
    std::unique_ptr<coro::scheduler>&    scheduler,
    std::stop_token                      stop_tok,
    std::chrono::milliseconds            interval = std::chrono::milliseconds{1000}
) -> coro::task<void>
{
    // -------------------------------------------------------------------------
    // Schedule this coroutine onto the scheduler's thread pool
    // LEARNING: This transfers execution from the calling thread to one of the
    // scheduler's worker threads. After this co_await, we're on a pool thread.
    // This is crucial: coro::scheduler needs to know about this task to manage
    // its I/O and timer events.
    // -------------------------------------------------------------------------
    co_await scheduler->schedule();

    // -------------------------------------------------------------------------
    // Simulated quote generation state
    // LEARNING: In a real system, this would be replaced by:
    //   auto client = coro::net::tcp::client{scheduler, endpoint};
    //   co_await client.connect();
    //   co_await client.write_all(request);
    //   auto [status, data] = co_await client.read_some(buffer);
    // For now, we simulate with deterministic data to avoid network dependency.
    // -------------------------------------------------------------------------
    auto hash = std::hash<std::string>{}(symbol);
    auto base_price = 100.0 + static_cast<double>(hash % 900);
    auto tick_count = uint64_t{0};

    // Use fprintf for logging — no std::cout (no mutex contention)
    // LEARNING: In HFT, even logging must be low-latency:
    // - fprintf to stderr is unbuffered, no global lock on std::cout
    // - Real systems use lock-free ring buffers to a logger thread
    // - Or binary logging: write raw struct to mmap'd file
    fprintf(stderr, "[poller] %-6s started (interval=%lldms)\n",
            symbol.c_str(), static_cast<long long>(interval.count()));

    // -------------------------------------------------------------------------
    // Main Polling Loop
    // LEARNING: This loop demonstrates the key difference between coroutines
    // and threads:
    //
    // WITH THREADS: sleep_for(1s) blocks the entire thread — wasteful.
    //   You'd need 1000 threads for 1000 symbols = massive stack memory.
    //
    // WITH COROUTINES: yield_for(1s) suspends THIS coroutine only.
    //   The thread goes back to process other coroutines.
    //   1000 symbols can share 4 threads = massive efficiency gain.
    //
    // CRITICAL: NEVER use std::this_thread::sleep_for() inside coroutines!
    //   It blocks the entire thread, preventing other coroutines from running.
    //   Always use scheduler->yield_for() or schedule_after() instead.
    // -------------------------------------------------------------------------
    while (!stop_tok.stop_requested())
    {
        ++tick_count;

        // ---------------------------------------------------------------------
        // Simulate receiving market data
        // TODO: Replace with real async HTTP/WebSocket poll:
        //   co_await client.write_all(build_http_get(symbol));
        //   auto [status, span] = co_await client.read_some(buffer);
        //   auto quote = simdjson::parse(span); // zero-allocation parse
        // ---------------------------------------------------------------------
        auto now_ns = static_cast<uint64_t>(
            std::chrono::steady_clock::now().time_since_epoch().count()
        );

        quote_data quote{
            .symbol       = symbol,
            .bid          = base_price + (tick_count % 10) * 0.01,
            .ask          = base_price + (tick_count % 10) * 0.01 + 0.05,
            .timestamp_ns = now_ns,
            .bid_size     = static_cast<uint32_t>(100 * ((tick_count % 5) + 1)),
            .ask_size     = static_cast<uint32_t>(100 * ((tick_count % 7) + 1)),
        };

        // ---------------------------------------------------------------------
        // TODO: Push quote to lock-free ring buffer for strategy engine:
        //   co_await ring_buffer.produce(std::move(quote));
        //
        // Or publish to a lock-free SPSC queue:
        //   while (!queue.try_push(std::move(quote))) { co_await scheduler->yield(); }
        // ---------------------------------------------------------------------

        // Low-latency logging: only print every 5th tick
        if (tick_count % 5 == 1)
        {
            fprintf(stderr,
                    "[poller] %-6s tick #%llu  bid=%.2f ask=%.2f  (bid_sz=%u ask_sz=%u)\n",
                    symbol.c_str(), static_cast<unsigned long long>(tick_count),
                    quote.bid, quote.ask,
                    quote.bid_size, quote.ask_size);
        }

        // ---------------------------------------------------------------------
        // SUSPEND: yield_for() suspends this coroutine for `interval`
        // LEARNING: This is the magic of coroutines for I/O-bound work:
        // - The coroutine frame is stored in the scheduler's timer heap
        // - The underlying thread is FREE to run other coroutines
        // - After `interval`, the scheduler resumes us on any pool thread
        // - NO thread is blocked, NO context switch overhead
        //
        // Implementation detail: scheduler uses epoll/kqueue timerfd (Linux)
        // or kevent timers (macOS) — kernel-level efficiency.
        // ---------------------------------------------------------------------
        co_await scheduler->yield_for(interval);
    }

    fprintf(stderr, "[poller] %-6s stopped after %llu ticks (shutdown requested)\n",
            symbol.c_str(), static_cast<unsigned long long>(tick_count));

    // -------------------------------------------------------------------------
    // co_return ends the coroutine and signals any awaiting task
    // LEARNING: For coro::task<void>, co_return is equivalent to co_return {}
    // The coroutine frame is destroyed after this point.
    // If anyone is co_await-ing this task, they will be resumed.
    // -------------------------------------------------------------------------
    co_return;
}

// ==============================================================================
// make_poller_task() — Wrapper that creates a poller task for when_all
// ==============================================================================
// LEARNING: This factory function demonstrates an important pattern:
// Tasks are LAZY — they don't execute until awaited.
// When you create a task and store it in a vector, it's just a handle.
// Only when passed to coro::when_all() and sync_wait()'d do they start.
//
// This is fundamentally different from std::async which starts immediately.
// ==============================================================================
inline auto make_poller_task(
    std::string                       symbol,
    std::unique_ptr<coro::scheduler>& scheduler,
    std::stop_token                   stop_tok,
    std::chrono::milliseconds         interval = std::chrono::milliseconds{1000}
) -> coro::task<void>
{
    // Simply forward — the task is created but NOT yet executing
    co_await poll_symbol(std::move(symbol), scheduler, stop_tok, interval);
}
