#pragma once
// ==============================================================================
// http_server.hpp — Coroutine-based TCP server for market data queries
// ==============================================================================
//
// LEARNING OBJECTIVES:
// 1. How coro::net::tcp::server accepts connections asynchronously
// 2. How coro::net::tcp::client reads/writes data within coroutines
// 3. Why spawn_detached() is used for per-connection handlers
// 4. How to structure a connection handler as a coroutine
// 5. Building block for HTTP/JSON API in a low-latency system
//
// ARCHITECTURE:
// - One accept-loop coroutine listens for new connections
// - Each accepted connection gets its own handler coroutine via spawn_detached()
// - The handler coroutine is "fire and forget" — scheduler owns its lifetime
// - All I/O is non-blocking and cooperatively multitasked
//
// FUTURE IMPROVEMENTS:
// - Parse HTTP requests properly (or use llhttp for zero-copy parsing)
// - Add /opportunities endpoint that reads from the shared quote store
// - Add /health endpoint for load balancer health checks
// - Rate limiting via coro::semaphore
// - Connection limiting to prevent resource exhaustion
// - TLS support via coro::net::tls::server (OpenSSL)
// ==============================================================================

#include <coro/coro.hpp>
#include <cstdio>
#include <string>
#include <string_view>

// ==============================================================================
// HTTP Response Builder (Minimal)
// ==============================================================================
// LEARNING: In a real system, you'd use a proper HTTP parser (llhttp, picohttpparser)
// and response builder. This is the simplest possible HTTP/1.1 response.
// For low-latency systems, pre-allocate response headers as compile-time constants.
// ==============================================================================
namespace http
{
    // Minimal HTTP/1.1 200 OK response with Content-Length
    // LEARNING: Content-Length is critical — without it, the client hangs
    // waiting for more data (chunked encoding or connection close).
    static std::string make_response(
        std::string_view body,
        std::string_view content_type = "application/json")
    {
        // Pre-compute all headers to avoid multiple allocations
        // In production: use std::format or fmt::format for zero-copy
        std::string response;
        response.reserve(256 + body.size());

        response.append("HTTP/1.1 200 OK\r\n");
        response.append("Content-Type: ");
        response.append(content_type);
        response.append("\r\n");
        response.append("Content-Length: ");
        response.append(std::to_string(body.size()));
        response.append("\r\n");
        response.append("Connection: close\r\n");  // No keep-alive for simplicity
        response.append("Server: stock-poller-coro/1.0\r\n");
        response.append("\r\n");
        response.append(body);

        return response;
    }

    static std::string make_404_response()
    {
        static constexpr auto body = R"({"error":"not found"})";
        std::string response;
        response.reserve(128);
        response.append("HTTP/1.1 404 Not Found\r\n");
        response.append("Content-Type: application/json\r\n");
        response.append("Content-Length: ");
        response.append(std::to_string(std::string_view{body}.size()));
        response.append("\r\n");
        response.append("Connection: close\r\n");
        response.append("\r\n");
        response.append(body);
        return response;
    }

} // namespace http

// ==============================================================================
// handle_connection() — Per-connection coroutine handler
// ==============================================================================
//
// LEARNING: This coroutine is spawned via scheduler->spawn_detached() for each
// new TCP connection. Key concepts:
//
// 1. OWNERSHIP: The client is moved into this function by value (not reference!)
//    This is CRITICAL — the accept() loop returns a temporary, and we need to
//    own the client for the lifetime of this connection.
//
// 2. BUFFER MANAGEMENT: We use a fixed 4KB buffer per connection.
//    In HFT systems, you'd use:
//    - A pre-allocated buffer pool (std::pmr::memory_resource)
//    - io_uring for zero-copy reads (Linux only)
//    - Recv-side buffer packing to reduce syscalls
//
// 3. SPAWN_DETACHED: The scheduler takes ownership of this task's lifetime.
//    When the coroutine completes (co_return), the scheduler cleans up.
//    This is the coroutine equivalent of "fire and forget" thread creation.
//
// ==============================================================================
auto handle_connection(
    coro::net::tcp::client client
) -> coro::task<void>
{
    // Buffer for reading client request
    // LEARNING: Fixed-size buffer avoids heap allocation per read.
    // In production: use a buffer pool or arena allocator.
    std::string buffer(4096, '\0');

    // -------------------------------------------------------------------------
    // Read the HTTP request
    // LEARNING: read_some() returns a pair: [read_status, read_span]
    // - read_status.is_ok()    → data received successfully
    // - read_status.is_closed() → client disconnected
    // - read_span is a view into our buffer for the bytes actually read
    //
    // IMPORTANT: This is read_SOME, not read_all. For HTTP you'd need to
    // loop until you see \r\n\r\n (end of headers). For this educational
    // example, one read is sufficient.
    // -------------------------------------------------------------------------
    auto [read_status, read_bytes] = co_await client.read_some(buffer);

    if (!read_status.is_ok())
    {
        // Client disconnected or error — just return, no cleanup needed
        // LEARNING: RAII ensures buffer is freed when coroutine frame is destroyed
        co_return;
    }

    // Trim buffer to actual bytes received
    std::string_view request{buffer.data(), read_bytes.size()};

    // -------------------------------------------------------------------------
    // Minimal HTTP Routing
    // LEARNING: Real systems use a proper HTTP parser + router.
    // This is the simplest possible path matching for learning purposes.
    // We look for "GET /" in the raw request — no header parsing.
    // -------------------------------------------------------------------------
    auto response = std::string{};

    if (request.starts_with("GET / "))
    {
        // ---------------------------------------------------------------------
        // Health check endpoint
        // TODO: Add /opportunities endpoint that reads from shared quote store:
        //   auto quotes = co_await quote_store.snapshot();
        //   auto json = format_opportunities(quotes);
        //   response = http::make_response(json);
        // ---------------------------------------------------------------------
        auto body = std::string_view{R"({"status":"ok","service":"stock-poller-coro","endpoints":["/","/health","/opportunities"]})"};
        response = http::make_response(body);
    }
    else if (request.starts_with("GET /health"))
    {
        auto body = std::string_view{R"({"status":"healthy","uptime_seconds":0})"};
        response = http::make_response(body);
    }
    else if (request.starts_with("GET /opportunities"))
    {
        // -----------------------------------------------------------------
        // TODO: This is the placeholder for the real opportunities endpoint.
        //
        // In a complete system, this would:
        // 1. Acquire shared lock on the quote store (coro::shared_mutex)
        // 2. Scan for symbols with wide bid-ask spreads
        // 3. Apply strategy filters (delta-neutral, volatility, etc.)
        // 4. Return JSON array of opportunities
        //
        // Example implementation:
        //   auto lock = co_await shared_mutex.scoped_lock_shared(task_fn);
        //   auto opportunities = strategy::scan(quote_store);
        //   auto json = simdjson::format(opportunities);
        // -----------------------------------------------------------------
        auto body = std::string_view{R"({"opportunities":[],"message":"placeholder - implement with real quote store"})"};
        response = http::make_response(body);
    }
    else
    {
        response = http::make_404_response();
    }

    // -------------------------------------------------------------------------
    // Write HTTP response
    // LEARNING: write_all() ensures ALL bytes are sent (handles partial writes).
    // It loops internally until the entire buffer is written or an error occurs.
    // Returns [write_status, unsent_span] — unsent should be empty on success.
    // -------------------------------------------------------------------------
    auto [write_status, unsent] = co_await client.write_all(response);

    if (!write_status.is_ok())
    {
        fprintf(stderr, "[http] write error: %s\n", write_status.message().data());
    }

    // co_return destroys the coroutine frame, closes the TCP connection (RAII)
    co_return;
}

// ==============================================================================
// run_http_server() — TCP accept-loop coroutine
// ==============================================================================
//
// LEARNING: This is the main server loop. Architecture:
//
// 1. Create tcp::server BEFORE scheduling — binds/listens immediately
// 2. Schedule onto scheduler — transfers to a worker thread
// 3. Infinite accept loop — each accept() suspends until a connection arrives
// 4. spawn_detached() for each connection — scheduler manages lifetime
//
// PATTERN: This is the "accept-and-dispatch" pattern used by all async servers.
// The key insight is that accept() is itself an async operation — it suspends
// the coroutine until a connection arrives, freeing the thread for other work.
//
// CRITICAL: The server object must be created BEFORE co_await scheduler->schedule()
// because the server constructor calls socket(), bind(), and listen() — all
// synchronous syscalls that must complete before we start accepting.
//
// ==============================================================================
auto run_http_server(
    std::unique_ptr<coro::scheduler>& scheduler,
    std::string_view                  host = "127.0.0.1",
    uint16_t                          port = 8080,
    std::stop_token                   stop_tok = {}
) -> coro::task<void>
{
    // -------------------------------------------------------------------------
    // Create TCP server: socket + bind + listen
    // LEARNING: This is done BEFORE scheduling onto the thread pool because:
    // - We need the socket to be listening before any client can connect
    // - If done after schedule(), there's a race: client might try to connect
    //   before the server is ready (connection refused)
    // - The server takes a reference to the scheduler for I/O event registration
    // -------------------------------------------------------------------------
    coro::net::tcp::server server{scheduler, {host, port}};

    // Now schedule this task onto the scheduler's thread pool
    co_await scheduler->schedule();

    fprintf(stderr, "[http] Server listening on %s:%u\n",
            std::string{host}.c_str(), port);

    // -------------------------------------------------------------------------
    // Accept Loop
    // LEARNING: co_await server.accept() suspends until a connection arrives.
    // It returns coro::expected<coro::net::tcp::client> — a result type that
    // either contains a valid client or an error.
    //
    // This is the C++23 std::expected pattern for error handling without
    // exceptions. In libcoro, it's similar to Rust's Result<T, E>.
    // -------------------------------------------------------------------------
    while (!stop_tok.stop_requested())
    {
        auto client_result = co_await server.accept();

        if (!client_result)
        {
            fprintf(stderr, "[http] accept error: %s\n",
                    client_result.error().message().data());
            continue;  // Don't stop the server on one bad accept
        }

        // -----------------------------------------------------------------
        // Dispatch connection to its own coroutine
        // LEARNING: spawn_detached() does THREE things:
        // 1. Moves the task into the scheduler's ownership
        // 2. Schedules the task to run on the thread pool
        // 3. Guarantees the task runs to completion (scheduler owns lifetime)
        //
        // The handle_connection coroutine will run on any available worker thread.
        // This is how you achieve massive concurrency with minimal threads:
        // - 10,000 connections handled by 4 threads
        // - Each connection is a lightweight coroutine, not a heavyweight thread
        // -----------------------------------------------------------------
        scheduler->spawn_detached(handle_connection(std::move(*client_result)));
    }

    fprintf(stderr, "[http] Server shutting down\n");
    co_return;
}
