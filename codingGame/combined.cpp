// ============================================================
// Coding Game Interview Questions (Q1–Q30)
// ============================================================

// --- 45-second questions ---
// 1. Power of Two: bool isPowerOfTwo(long long n) — true if n is power of 2 (including 1), n=0 → false
// 2. Count Set Bits: int countBits(unsigned int n) — number of 1-bits (Hamming weight)
// 3. Reverse Integer: Reverse digits of 32-bit signed int, return 0 on overflow

// --- 2–3 minute questions ---
// 4. Quadratic Solver: Read a,b,c doubles. Print number of real roots (0/1/2) and roots (6 decimal places)
// 5. Combination Count: Read n,k (1≤k≤n≤20). Output C(n,k) without overflow
// 6. Closest Pair Distance: Read n points (x,y ints). Output squared Euclidean distance of closest pair

// --- Short problems ---
// 7. Grid Shortest Path: W×H grid, (0,0)→(W-1,H-1), 4-directional. Output min moves
// 8. Stock Profit: Array of prices. Max profit from one buy + one sell (0 if none)
// 9. Run-Length Encoding: Encode string then decode to verify (use delimiter to handle digits)

// --- Medium problems ---
// 10. Tiling Rectangle: W×H rectangle. Min number of integer squares to tile (greedy OK)
// 11. BFS Shortest Path: Grid with obstacles (#), start (S), end (E). Output path length or -1

// --- Systems / HFT ---
// 12. Lock-Free SPSC Ring Buffer: 1024-slot circular buffer, atomic push/pop, no mutexes

// --- Bit Manipulation / Math ---
// 13. Bitwise AND of Range: AND of all numbers from left to right (inclusive).
// 14. Missing Number: Array of n distinct ints in [0, n], find missing one in O(1) extra space.
// 15. Palindrome Number: bool isPalindrome(int x). Negative numbers are not palindromes.
// 16. Hamming Distance: Number of positions where bits differ between x and y.
// 17. Valid Parentheses: String of '(){}[]', determine if properly closed.
// 18. Add Binary: Sum of two binary strings as binary string.
// 19. Single Number: Every element appears twice except one. Find it in O(n) time, O(1) space.
// 20. Climbing Stairs: n steps, 1 or 2 at a time. Number of distinct ways to reach top.

// --- Arrays / Greedy ---
// 21. Maximum Subarray Sum (Kadane): Contiguous subarray with largest sum.
// 22. Best Time to Buy and Sell Stock II: Unlimited transactions, max profit.
// 23. Move Zeroes: Move all 0's to end, in-place, O(1) extra space.

// --- Strings / Two-Pointer ---
// 24. Longest Palindromic Substring: Return longest palindromic substring (len ≤ 1000).
// 25. Merge Intervals: Merge overlapping intervals, return sorted list.
// 26. Container With Most Water: Max area between two lines.
// 27. Group Anagrams: Group anagrams together.
// 28. Product of Array Except Self: No division, O(n) time.
// 29. Rotate Array: Rotate right by k steps, in-place, O(1) extra space.
// 30. Valid Sudoku: Validate 9×9 board, use bit manipulation.

// 45-second questions (quick fill-in / one-liner)
// 31. Count Trailing Zeros in Factorial
// Write int trailingZeros(int n) — return the number of trailing zeros in n!.
// 32. Power Function (Fast Exponentiation)
// Write long long power(long long base, int exp) — compute base^exp efficiently (mod 10^9+7 if needed).
// 33. Integer to Roman
// Write string intToRoman(int num) (1 ≤ num ≤ 3999).
// 34. Roman to Integer
// Write int romanToInt(string s).
// 35. Reverse Bits
// Write uint32_t reverseBits(uint32_t n) — reverse all 32 bits.
// 36. Number of 1 Bits (again, different version)
// int hammingWeight(uint32_t n) — but must be O(1) time with bit tricks (n & (n-1) loop).

// 37. Implement Circular Buffer (Fixed Size Queue)
// Implement a fixed-size circular buffer (capacity given) with push(), pop(), peek(), isFull(), isEmpty(). Use modulo arithmetic.
// 38. LRU Cache (Simple Version)
// Design a Least Recently Used cache with get(key) and put(key, value). Capacity given. (Use ordered map or doubly linked list + hash).
// 39. Memory Pool Allocator
// Implement a simple fixed-size memory pool that can allocate and free blocks of a given size quickly (HFT classic).
// 40. Fast Input Parser
// Read a very large line of space-separated integers (up to 10^6 numbers) as fast as possible and compute their sum. (Use custom fast input, avoid cin with no sync).
// 41. Thread-Safe Counter
// Implement a lock-free incrementing counter using std::atomic.
// 42. Remove Duplicates from Sorted Array
// Remove duplicates in-place so that each unique element appears only once. Return the new length.
// 43. Find Peak Element
// Find a peak element (greater than neighbors) in an array. Can be multiple peaks, return any.
// 44. Kth Largest Element
// Find the kth largest element in an unsorted array. (Can use quickselect or heap).

#include <iostream>
#include <cmath>
#include <vector>
#include <queue>
#include <string>
#include <atomic>
#include <thread>
#include <iomanip>
#include <limits>
#include <cstdint>
#include <algorithm>
#include <numeric>
#include <bitset>
#include <unordered_set>
#include <unordered_map>
#include <optional>
#include <list>

// ============================================================
// Q12: Lock-Free SPSC Ring Buffer
// ============================================================
// Single-producer single-consumer ring buffer using atomics.
// acquire/release ordering ensures buffer writes are visible before
// head/tail updates. alignas(64) prevents false sharing between cores.

struct alignas(64) LockFreeRingBuffer {
    static constexpr size_t SIZE = 1024;

    alignas(64) std::atomic<size_t> head{0};
    alignas(64) std::atomic<size_t> tail{0};

    uint64_t buffer[SIZE]{};

    bool push(uint64_t value) {
        size_t current_head = head.load(std::memory_order_relaxed);
        size_t next_head = (current_head + 1) & (SIZE - 1);
        if (next_head == tail.load(std::memory_order_acquire)) {
            return false;
        }
        buffer[current_head] = value;
        head.store(next_head, std::memory_order_release);
        return true;
    }

    bool pop(uint64_t &value) {
        size_t current_tail = tail.load(std::memory_order_relaxed);
        if (current_tail == head.load(std::memory_order_acquire)) {
            return false;
        }
        value = buffer[current_tail];
        tail.store((current_tail + 1) & (SIZE - 1), std::memory_order_release);
        return true;
    }
};

// ============================================================
// Q1–Q3: 45-Second Questions
// ============================================================
// Q1: n & (n-1) clears lowest set bit — powers of 2 have exactly one bit set.
// Q2: Brian Kernighan's — iterates only over set bits (faster than naive 32-bit loop).
// Q3: INT_MIN special-cased (abs overflows); overflow check before each multiply-add.

void question1() {
    auto isPowerOfTwo = [](long long n) {
        return n > 0 && (n & (n - 1)) == 0;
    };

    long long x;
    std::cin >> x;
    std::cout << std::boolalpha << isPowerOfTwo(x) << '\n';
}

void question2() {
    auto countBits = [](unsigned int n) {
        int count = 0;
        while (n) {
            n &= n - 1;
            ++count;
        }
        return count;
    };

    unsigned int x;
    std::cin >> x;
    std::cout << countBits(x) << '\n';
}

// Q3: Full reversal with per-digit overflow check — works entirely in int-width arithmetic.
// INT_MIN special-cased because abs(INT_MIN) is UB (|INT_MIN| > INT_MAX in two's complement).
// Relevant when constrained to int-width accumulator (e.g., embedded systems).

void question3() {
    auto reverseInteger = [](int x) -> int {
        if (x == std::numeric_limits<int>::min()) return 0;
        int sign = (x < 0) ? -1 : 1;
        x = std::abs(x);
        int reversed = 0;
        while (x > 0) {
            int digit = x % 10;
            if (reversed > (std::numeric_limits<int>::max() - digit) / 10) return 0;
            reversed = reversed * 10 + digit;
            x /= 10;
        }
        return reversed * sign;
    };

    int x;
    std::cin >> x;
    std::cout << reverseInteger(x) << '\n';
}

// Q3a: Simplified — long long accumulator absorbs overflow; no abs/sign/INT_MIN needed.
// C++ truncation-toward-zero makes n%10 carry sign naturally (e.g., -123%10 = -3).
// Single range check at the end replaces per-digit overflow guard.
// Trade-off: uses 8-byte accumulator instead of 4.

void question3a() {
    int x;
    std::cin >> x;
    long long rev = 0;
    for (int n = x; n; n /= 10)
        rev = rev * 10 + n % 10;
    std::cout << (rev < INT_MIN || rev > INT_MAX ? 0 : rev) << '\n';
}

// ============================================================
// Q4: Quadratic Solver
// ============================================================
// Epsilon-based discriminant classification avoids floating-point equality.
// Handles degenerate cases: a=0 (linear), b=0 (no solution).

void question4() {
    double a, b, c;
    std::cin >> a >> b >> c;

    constexpr double eps = 1e-9;
    std::cout << std::fixed << std::setprecision(6);

    if (std::abs(a) < eps) {
        if (std::abs(b) < eps) {
            std::cout << "0\n";
        } else {
            std::cout << "1 " << (-c / b) << '\n';
        }
    } else {
        double d = b * b - 4 * a * c;
        if (d > eps) {
            double sqrt_d = std::sqrt(d);
            std::cout << "2 " << (-b + sqrt_d) / (2 * a) << " " << (-b - sqrt_d) / (2 * a) << '\n';
        } else if (d < -eps) {
            std::cout << "0\n";
        } else {
            std::cout << "1 " << (-b / (2 * a)) << '\n';
        }
    }
}

// ============================================================
// Q5: Combination Count
// ============================================================
// Multiplicative form: C(n,k) = n/1 * (n-1)/2 * ... * (n-k+1)/k
// Divisibility: product at step i is always a multiple of i!, so
// integer division never truncates — no floating point needed.
// Symmetry: C(n,k) = C(n,n-k) minimizes iterations (e.g. C(20,18) → C(20,2)).
// Overflow: safe for n ≤ 20 since C(20,10) = 184756 fits in unsigned long long.
// Why not factorials? 20! ≈ 2.4e18 barely fits ULLONG_MAX, and n!/(k!(n-k)!)
// overflows at intermediate steps. Iterative product avoids that.

void question5() {
    int n, k;
    std::cin >> n >> k;

    if (k < 0 || k > n) {
        std::cout << "0\n";
        return;
    }
    if (k > n - k) k = n - k;

    unsigned long long result = 1;
    for (int i = 1; i <= k; ++i) {
        result = result * (n - i + 1) / i;
    }
    std::cout << result << '\n';
}

// ============================================================
// Q6: Closest Pair Distance (squared)
// ============================================================
// O(n^2) brute force — acceptable for small n. Returns squared distance
// to avoid costly sqrt. Casts to unsigned long long to prevent overflow.

void question6() {
    int n;
    std::cin >> n;

    std::vector<std::pair<int, int>> points(n);
    for (int i = 0; i < n; ++i) {
        std::cin >> points[i].first >> points[i].second;
    }

    unsigned long long min_sq = std::numeric_limits<unsigned long long>::max();
    for (size_t i = 0; i + 1 < points.size(); ++i) {
        for (size_t j = i + 1; j < points.size(); ++j) {
            long long dx = points[i].first - points[j].first;
            long long dy = points[i].second - points[j].second;
            unsigned long long sq = static_cast<unsigned long long>(dx * dx) + static_cast<unsigned long long>(dy * dy);
            min_sq = std::min(min_sq, sq);
        }
    }
    std::cout << min_sq << '\n';
}

// ============================================================
// Q7: Grid Shortest Path (No Obstacles)
// ============================================================
// Pure combinatorics: must move right (W-1) and down (H-1) times.
// No BFS needed — answer is always (W-1) + (H-1).

void question7() {
    int W, H;
    std::cin >> W >> H;
    std::cout << (W - 1) + (H - 1) << '\n';
}

// ============================================================
// Q8: Stock Profit
// ============================================================
// Two-pass: stores all prices in vector, then scans for max profit.
// O(n) time, O(n) space — useful if you need to inspect prices later
// (e.g., return the buy/sell indices). Wasteful if only profit is needed.

void question8() {
    int n;
    std::cin >> n;

    if (n <= 1) {
        std::cout << "0\n";
        return;
    }

    std::vector<int> prices(n);
    for (int i = 0; i < n; ++i) {
        std::cin >> prices[i];
    }

    int min_price = prices[0];
    int max_profit = 0;
    for (int i = 1; i < n; ++i) {
        min_price = std::min(min_price, prices[i]);
        max_profit = std::max(max_profit, prices[i] - min_price);
    }
    std::cout << max_profit << '\n';
}

// Q8a: Single-pass streaming — no vector needed, O(1) space.
// Each price is used exactly once (compare to min, compute profit),
// so there's no reason to store it. Read and process on the fly.

void question8a() {
    int n;
    std::cin >> n;
    if (n <= 1) {
        std::cout << "0\n";
        return;
    }

    int price;
    std::cin >> price;
    int min_price = price;
    int max_profit = 0;
    for (int i = 1; i < n; ++i) {
        std::cin >> price;
        min_price = std::min(min_price, price);
        max_profit = std::max(max_profit, price - min_price);
    }
    std::cout << max_profit << '\n';
}

// ============================================================
// Q9: Run-Length Encoding
// ============================================================
// Fixed-width: each run is exactly <char><1-digit-count> (max 9 per run).
// No delimiter needed — decoder simply reads 2 chars at a time.
// Handles digits and any characters unambiguously. Runs >9 split into
// multiple entries (e.g., 12 a's → "a9a3").

void question9() {
    std::string S;
    std::cin >> S;

    std::string encoded;
    for (size_t i = 0; i < S.size(); ) {
        char ch = S[i];
        size_t count = 1;
        while (i + count < S.size() && S[i + count] == ch && count < 9) {
            ++count;
        }
        encoded += ch + std::to_string(count);
        i += count;
    }
    std::cout << encoded << '\n';

    std::string decoded;
    for (size_t i = 0; i + 1 < encoded.size(); i += 2) {
        decoded += std::string(encoded[i + 1] - '0', encoded[i]);
    }
    std::cout << decoded << '\n';
}

// ============================================================
// Q10: Tiling Rectangle with Squares (greedy)
// ============================================================
// Euclidean algorithm: always place the largest square (min(W,H))^2,
// then reduce. Greedy is optimal for integer square tiling.

void question10() {
    unsigned int W, H;
    std::cin >> W >> H;

    unsigned int count = 0;
    while (W > 0 && H > 0) {
        if (W < H) std::swap(W, H);
        count += W / H;
        W = W % H;
    }
    std::cout << count << '\n';
}

// ============================================================
// Q11: BFS Shortest Path with Obstacles
// ============================================================
// BFS guarantees shortest path on unweighted grids. Visited array
// prevents re-exploration — O(rows*cols) time and space.

void question11() {
    int rows, cols;
    std::cin >> rows >> cols;

    std::vector<std::string> grid(rows);
    for (int i = 0; i < rows; ++i) {
        std::cin >> grid[i];
    }

    std::pair<int, int> start, end;
    for (int r = 0; r < rows; ++r) {
        for (int c = 0; c < cols; ++c) {
            if (grid[r][c] == 'S') start = {r, c};
            if (grid[r][c] == 'E') end = {r, c};
        }
    }

    std::vector<bool> visited(rows * cols, false);
    std::queue<std::tuple<int, int, int>> q;
    q.push({start.first, start.second, 0});
    visited[start.first * cols + start.second] = true;

    int shortest_path = -1;
    while (!q.empty()) {
        auto [r, c, dist] = q.front();
        q.pop();
        if (std::make_pair(r, c) == end) {
            shortest_path = dist;
            break;
        }
        constexpr int dr[] = {1, -1, 0, 0};
        constexpr int dc[] = {0, 0, 1, -1};
        for (int i = 0; i < 4; ++i) {
            int nr = r + dr[i], nc = c + dc[i];
            if (nr >= 0 && nr < rows && nc >= 0 && nc < cols &&
                grid[nr][nc] != '#' && !visited[nr * cols + nc]) {
                visited[nr * cols + nc] = true;
                q.push({nr, nc, dist + 1});
            }
        }
    }
    std::cout << shortest_path << '\n';
}

// ============================================================
// Q12: Lock-Free SPSC Ring Buffer (demo)
// ============================================================
// Producer/consumer threads spin on full/empty — acceptable for demo.
// In production, consider adding backoff or notification.

void question12() {
    LockFreeRingBuffer rb;

    constexpr int N = 500;
    std::thread producer([&]() {
        for (int i = 0; i < N; ++i) {
            while (!rb.push(static_cast<uint64_t>(i)))
                ;
        }
    });

    std::thread consumer([&]() {
        uint64_t value;
        for (int i = 0; i < N; ++i) {
            while (!rb.pop(value))
                ;
            std::cout << value << '\n';
        }
    });

    producer.join();
    consumer.join();
}

// ============================================================
// Q13: Bitwise AND of Range
// ============================================================
// Naive loop with early exit when result hits 0.
// Optimal: find common prefix of left/right, fill rest with 0 — O(1).

void question13() {
    std::cout << "Enter left and right for rangeBitwiseAnd: ";
    int left, right;
    std::cin >> left >> right;
    if (left > right) {
        std::cout << "Invalid input: left should be <= right.\n";
        return;
    }
    int answer = left;
    for (int i = left + 1; i <= right; ++i) {
        answer &= i;
        if (answer == 0) break;
    }
    std::cout << answer << '\n';
}

// ============================================================
// Q14: Missing Number
// ============================================================
// Sum formula: expected sum (n*(n+1)/2) minus actual sum reveals missing value.
// Alternative: XOR all values with [0..n] — overflow-safe but less intuitive.

void question14() {
    std::cout << "Enter n for missing number (array of size n with numbers from 0 to n): \n";
    int n;
    std::cin >> n;
    std::vector<int> nums(n);
    std::cout << "Enter " << n << " distinct integers in range [0, " << n << "]: ";
    for (int i = 0; i < n; ++i) {
        std::cin >> nums[i];
    }
    // Use long long to avoid overflow in n*(n+1)/2
    long long expected = static_cast<long long>(n) * (n + 1) / 2;
    long long actual = std::accumulate(nums.begin(), nums.end(), 0LL);
    std::cout << expected - actual << '\n';
}

// ============================================================
// Q15: Palindrome Number (full reversal)
// ============================================================
// Full reversal approach — risk of overflow on large numbers.
// Q15a uses half-reversal: compares only half the digits, no overflow possible.

void question15() {
    std::cout << "Enter an integer to check if it's a palindrome: ";
    int x;
    std::cin >> x;

    if (x < 0) {
        std::cout << "false\n";
        return;
    }

    int original = x, reversed = 0;
    while (x > 0) {
        reversed = reversed * 10 + x % 10;
        x /= 10;
    }
    std::cout << std::boolalpha << (original == reversed) << '\n';
}

// Q15a: Half-reversal — O(d/2), no overflow risk
void question15a() {
    int x;
    std::cin >> x;

    if (x < 0 || (x % 10 == 0 && x != 0)) {
        std::cout << "false\n";
        return;
    }

    int reversed_half = 0;
    while (x > reversed_half) {
        reversed_half = reversed_half * 10 + x % 10;
        x /= 10;
    }
    std::cout << std::boolalpha << (x == reversed_half || x == reversed_half / 10) << '\n';
}

// ============================================================
// Q16: Hamming Distance
// ============================================================
// XOR finds differing bits; bitset::count() computes popcount.
// Alternative: use n & (n-1) loop like Q2 for explicit bit manipulation.

void question16() {
    int x, y;
    std::cin >> x >> y;
    std::cout << std::bitset<32>(x ^ y).count() << '\n';
}

// ============================================================
// Q17: Valid Parentheses
// ============================================================
// Expected-close bracket stack: push the *closing* bracket for each opener.
// O(n) single pass with early exit on mismatch.

void question17() {
    std::cout << "Enter a string of parentheses to check if it's valid: ";
    std::string s;
    std::cin >> s;
    std::vector<char> stack;

    for (char c : s) {
        switch (c) {
            case '(': stack.push_back(')'); break;
            case '{': stack.push_back('}'); break;
            case '[': stack.push_back(']'); break;
            case ')':
            case '}':
            case ']':
                if (stack.empty() || stack.back() != c) {
                    std::cout << "false\n";
                    return;
                }
                stack.pop_back();
                break;
            default: break;
        }
    }
    std::cout << std::boolalpha << stack.empty() << '\n';
}

// ============================================================
// Q18: Add Binary
// ============================================================
// Column addition from LSB; carry propagates naturally.
// Reverse result at the end — no need to handle leading zeros.

void question18() {
    std::cout << "Enter two binary strings to add: ";
    std::string a, b;
    std::cin >> a >> b;
    std::string result;
    int carry = 0, i = a.size() - 1, j = b.size() - 1;
    while (i >= 0 || j >= 0 || carry) {
        int sum = carry;
        if (i >= 0) sum += a[i--] - '0';
        if (j >= 0) sum += b[j--] - '0';
        result.push_back((sum % 2) + '0');
        carry = sum / 2;
    }
    std::reverse(result.begin(), result.end());
    std::cout << result << '\n';
}

// ============================================================
// Q19: Single Number (XOR)
// ============================================================
// XOR cancels pairs (a ^ a = 0) — O(n) time, O(1) space.
// question19_set uses hash set — O(n) time, O(n/2) space, but more generalizable.

void question19() {
    int n;
    std::cin >> n;
    int single = 0;
    for (int i = 0; i < n; ++i) {
        int num;
        std::cin >> num;
        single ^= num;
    }
    std::cout << single << '\n';
}

// O(n) time, O(n/2) space alternative
void question19_set() {
    int n;
    std::cin >> n;
    std::unordered_set<int> seen;
    for (int i = 0; i < n; ++i) {
        int num;
        std::cin >> num;
        auto [it, inserted] = seen.insert(num);
        if (!inserted) seen.erase(it);
    }
    std::cout << *seen.begin() << '\n';
}

// ============================================================
// Q20: Climbing Stairs (O(1) space)
// ============================================================
// Fibonacci relation: ways(n) = ways(n-1) + ways(n-2).
// Iterative two-variable approach avoids O(n) stack from recursion.

void question20() {
    int n;
    std::cin >> n;
    if (n <= 0) {
        std::cout << "0\n";
        return;
    }
    if (n == 1) {
        std::cout << "1\n";
        return;
    }
    long long prev2 = 1, prev1 = 2;
    for (int i = 3; i <= n; ++i) {
        long long curr = prev1 + prev2;
        prev2 = prev1;
        prev1 = curr;
    }
    std::cout << prev1 << '\n';
}

// ============================================================
// Q21: Maximum Subarray Sum (Kadane)
// ============================================================
// Kadane's: reset running sum when negative (starting fresh is better).
// Global max tracks the best seen. O(n) time, O(1) space.

void question21() {
    std::cout << "Enter the number of elements in the array: ";
    int n;
    std::cin >> n;
    if (n <= 0) {
        std::cout << "Invalid input: n must be > 0.\n";
        return;
    }
    std::vector<int> nums(n);
    std::cout << "Enter the elements: ";
    for (int i = 0; i < n; ++i) {
        std::cin >> nums[i];
    }

    long long max_sum = nums[0], current_sum = nums[0];
    for (int i = 1; i < n; ++i) {
        current_sum = std::max(static_cast<long long>(nums[i]), current_sum + nums[i]);
        max_sum = std::max(max_sum, current_sum);
    }

    std::cout << "Maximum subarray sum: " << max_sum << '\n';
}

// ============================================================
// Q22: Best Time to Buy and Sell Stock II
// ============================================================
// Sum all positive deltas — equivalent to buying at every valley,
// selling at every peak. Greedy is optimal when unlimited transactions.

void question22() {
    std::cout << "Enter the number of days: ";
    int n;
    std::cin >> n;
    if (n <= 1) {
        std::cout << "Maximum profit: 0\n";
        return;
    }
    std::vector<int> prices(n);
    std::cout << "Enter the stock prices: ";
    for (int i = 0; i < n; ++i) {
        std::cin >> prices[i];
    }

    long long max_profit = 0;
    for (int i = 1; i < n; ++i) {
        if (prices[i] > prices[i - 1]) {
            max_profit += prices[i] - prices[i - 1];
        }
    }

    std::cout << "Maximum profit: " << max_profit << '\n';
}

// ============================================================
// Q23: Move Zeroes
// ============================================================
// Two-pointer: last_non_zero tracks write position. Swap guard (i != last_non_zero)
// avoids self-assignment. Preserves relative order of non-zero elements.

void question23() {
    std::cout << "Enter the number of elements in the array: ";
    int n;
    std::cin >> n;
    if (n <= 0) {
        std::cout << "Invalid input: n must be > 0.\n";
        return;
    }
    std::vector<int> nums(n);
    std::cout << "Enter the elements: ";
    for (int i = 0; i < n; ++i) {
        std::cin >> nums[i];
    }

    int last_non_zero = 0;
    for (int i = 0; i < n; ++i) {
        if (nums[i] != 0) {
            if (i != last_non_zero) {
                nums[last_non_zero] = nums[i];
                nums[i] = 0;
            }
            ++last_non_zero;
        }
    }

    std::cout << "Array after moving zeroes: [";
    for (size_t i = 0; i < nums.size(); ++i) {
        std::cout << nums[i];
        if (i < nums.size() - 1) std::cout << ", ";
    }
    std::cout << "]\n";
}

// ============================================================
// Q24: Longest Palindromic Substring (Expand Around Center)
// ============================================================
// O(n^2) time, O(1) space. Deduplicated with expand lambda.
// For n ≤ 1000 this is practical; Manacher's (Q24a) is O(n) but more complex.

void question24() {
    std::cout << "Enter a string to find its longest palindromic substring:\n";
    std::string s;
    std::getline(std::cin >> std::ws, s);  // Read entire line including spaces

    int n = static_cast<int>(s.size());
    if (n == 0) {
        std::cout << "Longest palindromic substring: \"\"\n";
        return;
    }

    int start = 0, max_len = 1;

    // Helper: expand from center [left, right], return palindrome length
    auto expand = [&](int left, int right) {
        while (left >= 0 && right < n && s[left] == s[right]) {
            int len = right - left + 1;
            if (len > max_len) {
                start = left;
                max_len = len;
            }
            --left;
            ++right;
        }
    };

    for (int i = 0; i < n; ++i) {
        expand(i, i);       // Odd-length palindrome (single center)
        expand(i, i + 1);   // Even-length palindrome (between two chars)
    }

    std::cout << "Longest palindromic substring: \"" << s.substr(start, max_len) << "\"\n";
}

// ============================================================
// Q24a: Longest Palindromic Substring (Manacher's Algorithm)
// ============================================================
// Manacher's algorithm finds the longest palindromic substring in O(n) time
// by leveraging the symmetry of palindromes to avoid redundant comparisons.
//
// Key insight: For a palindrome centered at c, the palindrome at a mirror
// position i' = 2c - i has the same radius (bounded by the palindrome's edge).
// This lets us "jump" past already-explored regions.
//
// Technique: Transform the string by inserting sentinels (e.g., '#') between
// characters so both odd and even palindromes are handled uniformly.
// Example: "abba" -> "^#a#b#b#a#$" (^ and $ are boundary guards)
//
// The algorithm maintains:
// - center: the center of the rightmost palindrome found so far
// - right:  the right edge of that palindrome
// - p[i]:   the radius of the palindrome centered at position i
//
// For each position i:
// 1. If i < right, initialize p[i] using its mirror i' = 2*center - i
//    (p[i] is at least min(p[i'], right - i) by symmetry)
// 2. Attempt to expand beyond the initial radius
// 3. If expansion pushes right edge further, update center and right
//
// After processing, max(p) gives the longest palindrome radius.
// The original substring indices are recovered by transforming back.
//
// Time: O(n) — each character is compared at most twice
// Space: O(n) — for the transformed string and radius array

void question24a() {
    std::cout << "Enter a string to find its longest palindromic substring (Manacher):\n";
    std::string s;
    std::getline(std::cin >> std::ws, s);

    if (s.empty()) {
        std::cout << "Longest palindromic substring: \"\"\n";
        return;
    }

    // Transform: insert '#' between chars, wrap with '^' and '$' as sentinels
    // This makes even-length palindromes behave like odd-length ones
    // Example: "aba" -> "^#a#b#a#$", palindrome centered at 'b' spans #a#b#a#
    std::string t = "^";
    for (char c : s) {
        t += '#';
        t += c;
    }
    t += '#';
    t += '$';

    int n = static_cast<int>(t.size());
    std::vector<int> p(n, 0);  // p[i] = radius of palindrome centered at i

    int center = 0, right = 0;  // Current rightmost palindrome's center and edge

    for (int i = 1; i < n - 1; ++i) {
        // Mirror position of i with respect to center
        // i_mirror = center - (i - center) = 2*center - i
        int i_mirror = 2 * center - i;

        // Case 1: i is within the current rightmost palindrome
        // By symmetry, p[i] is at least p[i_mirror], but cannot exceed right - i
        // (the palindrome cannot extend beyond right without verification)
        if (i < right) {
            p[i] = std::min(right - i, p[i_mirror]);
        }

        // Attempt to expand palindrome centered at i beyond known radius
        // Compare characters at positions (i - p[i] - 1) and (i + p[i] + 1)
        // The sentinels '^' at index 0 and '$' at index n-1 will terminate expansion
        while (t[i + p[i] + 1] == t[i - p[i] - 1]) {
            ++p[i];
        }

        // If palindrome centered at i extends beyond current right edge,
        // update center and right to this new rightmost palindrome
        if (i + p[i] > right) {
            center = i;
            right = i + p[i];
        }
    }

    // Find the maximum radius and its center position
    int max_radius = 0, center_idx = 0;
    for (int i = 1; i < n - 1; ++i) {
        if (p[i] > max_radius) {
            max_radius = p[i];
            center_idx = i;
        }
    }

    // Convert back to original string indices
    // In transformed string, actual characters are at odd indices (1, 3, 5, ...)
    // The center of the palindrome in original string:
    //   start = (center_idx - max_radius) / 2
    //   length = max_radius
    // Why? Each original character occupies 2 positions in t (e.g., 'a' at #a#)
    int start = (center_idx - max_radius) / 2;
    std::string result = s.substr(start, max_radius);

    std::cout << "Longest palindromic substring: \"" << result << "\"\n";
    std::cout << "Manacher: O(n) time, O(n) space. Radius array: ";
    for (int i = 1; i < n - 1; ++i) std::cout << p[i] << (i < n - 2 ? " " : "");
    std::cout << "\n";
}

// ============================================================
// Q25: Merge Intervals (In-Place)
// ============================================================
// In-place read/write pointer — O(1) extra space vs separate merged vector.
// Sort dominates at O(n log n); cannot improve without range constraints.

void question25() {
    std::cout << "Enter the number of intervals: ";
    int n;
    std::cin >> n;
    if (n <= 0) {
        std::cout << "Invalid input: n must be > 0.\n";
        return;
    }
    std::vector<std::pair<int, int>> intervals(n);
    std::cout << "Enter the intervals (start end):\n";
    for (int i = 0; i < n; ++i) {
        auto &[start, end] = intervals[i];
        std::cin >> start >> end;
        if (start > end) std::swap(start, end);  // Normalize: ensure start <= end
    }

    // O(n log n) — sort dominates; cannot do better without range constraints
    std::sort(intervals.begin(), intervals.end());

    // In-place merge: write result into front of intervals, then resize
    // O(1) extra space (vs O(n) with separate merged vector)
    int write = 0;
    for (int read = 1; read < n; ++read) {
        if (intervals[read].first <= intervals[write].second) {
            // Overlapping or touching — extend current interval's end
            intervals[write].second = std::max(intervals[write].second, intervals[read].second);
        } else {
            // Disjoint — advance write pointer and copy
            intervals[++write] = intervals[read];
        }
    }
    intervals.resize(write + 1);

    std::cout << "Merged intervals:\n";
    for (const auto &[start, end] : intervals) {
        std::cout << start << " " << end << '\n';
    }
}

// ============================================================
// Q26: Container With Most Water
// ============================================================
// Two-pointer greedy: discarding the shorter side is safe because
// width shrinks and min-height can't improve by keeping it.
// long long prevents overflow on height * width.

void question26() {
    std::cout << "Enter the number of lines: ";
    int n;
    std::cin >> n;
    if (n <= 1) {
        std::cout << "Invalid input: n must be > 1.\n";
        return;
    }
    std::vector<int> heights(n);
    std::cout << "Enter the heights of the lines:\n";
    for (int i = 0; i < n; ++i) {
        std::cin >> heights[i];
    }

    // Two-pointer: start at both ends, move the shorter side inward.
    // Correctness: discarding the shorter side is safe because any container
    // using that line with an inner line has less width and no more height.
    int left = 0, right = n - 1;
    long long max_area = 0;  // long long: height * width can overflow int
    while (left < right) {
        long long height = std::min(heights[left], heights[right]);
        long long width = right - left;
        max_area = std::max(max_area, height * width);
        if (heights[left] < heights[right]) {
            ++left;
        } else {
            --right;
        }
    }

    std::cout << "Maximum area: " << max_area << '\n';
}

// ============================================================
// Q27: Group Anagrams
// ============================================================
// Sorted string as hash key — O(k log k) per string where k = string length.
// Alternative: 26-int frequency count as key — O(k) but more complex encoding.

void question27() {
    std::cout << "Enter the number of strings: ";
    int n;
    std::cin >> n;
    if (n <= 0) {
        std::cout << "Invalid input: n must be > 0.\n";
        return;
    }
    std::vector<std::string> strs(n);
    std::cout << "Enter the strings:\n";
    for (int i = 0; i < n; ++i) {
        std::cin >> strs[i];
    }

    // Group anagrams using sorted string as key
    // O(n k log k) time, where k is max string length (sorting each string)
    // O(n k) space for groups
    std::unordered_map<std::string, std::vector<std::string>> groups;
    for (const auto &s : strs) {
        std::string key = s;
        std::sort(key.begin(), key.end());
        groups[key].push_back(s);
    }

    std::cout << "Grouped anagrams:\n";
    for (const auto &[key, group] : groups) {
        for (const auto &s : group) {
            std::cout << s << " ";
        }
        std::cout << '\n';
    }
}

// ============================================================
// Q28: Product of Array Except Self
// ============================================================
// Two-pass prefix/suffix — no division needed, handles zeros naturally.
// O(n) time, O(1) extra space (output array doesn't count per convention).

void question28() {
    std::cout << "Enter the number of elements in the array: ";
    int n;
    std::cin >> n;
    if (n <= 0) {
        std::cout << "Invalid input: n must be > 0.\n";
        return;
    }
    std::vector<int> nums(n);
    std::cout << "Enter the elements:\n";
    for (int i = 0; i < n; ++i) {
        std::cin >> nums[i];
    }

    // Product of array except self without division
    // O(n) time, O(1) extra space (output array doesn't count)
    std::vector<long long> output(n, 1);
    long long left_product = 1;
    for (int i = 0; i < n; ++i) {
        output[i] *= left_product;
        left_product *= nums[i];
    }
    long long right_product = 1;
    for (int i = n - 1; i >= 0; --i) {
        output[i] *= right_product;
        right_product *= nums[i];
    }

    std::cout << "Product of array except self:\n[";
    for (size_t i = 0; i < output.size(); ++i) {
        std::cout << output[i];
        if (i < output.size() - 1) std::cout << ", ";
    }
    std::cout << "]\n";
}

// ============================================================
// Q29: Rotate Array
// ============================================================
// Three-reverse technique — in-place O(1) extra space.
// Double modulo ((k%n)+n)%n handles negative k correctly in C++.

void question29() {
    std::cout << "Enter the number of elements in the array: ";
    int n;
    std::cin >> n;
    if (n <= 0) {
        std::cout << "Invalid input: n must be > 0.\n";
        return;
    }
    std::vector<int> nums(n);
    std::cout << "Enter the elements:\n";
    for (int i = 0; i < n; ++i) {
        std::cin >> nums[i];
    }
    std::cout << "Enter the number of steps to rotate: ";
    int k;
    std::cin >> k;
    k = ((k % n) + n) % n;  // Normalize: handles negative k and k >= n
    if (k == 0) {
        std::cout << "Array after rotation:\n[";
        for (size_t i = 0; i < nums.size(); ++i) {
            std::cout << nums[i];
            if (i < nums.size() - 1) std::cout << ", ";
        }
        std::cout << "]\n";
        return;
    }

    // Rotate right by k in-place via three reversals
    // O(n) time, O(1) extra space
    auto reverse = [&](int start, int end) {
        while (start < end) {
            std::swap(nums[start++], nums[end--]);
        }
    };

    reverse(0, n - 1);       // Reverse entire array
    reverse(0, k - 1);       // Reverse first k elements
    reverse(k, n - 1);       // Reverse remaining n-k elements

    std::cout << "Array after rotation:\n[";
    for (size_t i = 0; i < nums.size(); ++i) {
        std::cout << nums[i];
        if (i < nums.size() - 1) std::cout << ", ";
    }
    std::cout << "]\n";
}

// ============================================================
// Q30: Valid Sudoku
// ============================================================
// Bit manipulation: 3 arrays of 9 ints as 9-bit bitmasks (one per row/col/box).
// Single pass O(81) = O(1) time and space. (r/3)*3 + (c/3) maps cell to box.

void question30() {
    // Valid Sudoku: validate 9×9 board using bit manipulation
    // Each row, column, and 3×3 box uses an int as a 9-bit bitmask.
    // Bit i (0-indexed) represents digit (i+1).
    // Set bit: mask |= (1 << i), check duplicate: mask & (1 << i)

    std::cout << "Enter 9 rows of the Sudoku board (digits 1-9, '.' for empty):\n";
    std::vector<std::string> board(9);
    for (int i = 0; i < 9; ++i) {
        std::cin >> board[i];
    }

    int rows[9] = {}, cols[9] = {}, boxes[9] = {};

    for (int r = 0; r < 9; ++r) {
        for (int c = 0; c < 9; ++c) {
            if (board[r][c] == '.') continue;
            int bit = board[r][c] - '1';  // Map '1'-'9' to bit 0-8
            int mask = 1 << bit;
            int box = (r / 3) * 3 + (c / 3);  // Box index 0-8

            if ((rows[r] | cols[c] | boxes[box]) & mask) {
                // Bit already set — duplicate in row, column, or box
                std::cout << "false (duplicate at row " << r + 1
                          << ", col " << c + 1 << ")\n";
                return;
            }
            rows[r] |= mask;
            cols[c] |= mask;
            boxes[box] |= mask;
        }
    }

    std::cout << std::boolalpha << true << '\n';
}   

// ============================================================
// Q31: Trailing Zeros in Factorial
// ============================================================

void question31() {
    std::cout << "Enter n to count trailing zeros in n!: ";
    int n;
    std::cin >> n;

    // Legendre's formula: count factors of 5 in n!
    // n/5 + n/25 + n/125 + ... counts how many times 5 appears as a factor
    int count = 0;
    while (n > 0) {
        n /= 5;
        count += n;
    }

    std::cout << "Trailing zeros: " << count << '\n';
}

// ============================================================
// Q32: Power Function (Fast Exponentiation)
// ============================================================
// Binary exponentiation: O(log n) vs O(n) naive. Square-and-multiply.
// long long promotion handles INT_MIN; 0^-n guarded explicitly.

void question32() {
    std::cout << "Enter base (double) and exponent (int): ";
    double x;
    int n;
    std::cin >> x >> n;

    auto myPow = [](double x, int n) -> double {
        if (n == 0) return 1.0;
        long long N = n;  // Promote to long long to handle INT_MIN
        if (N < 0) {
            if (x == 0.0) return 0.0;  // 0^-n is undefined, return 0
            x = 1 / x;
            N = -N;
        }
        double result = 1.0;
        double current_product = x;
        for (long long i = N; i > 0; i /= 2) {
            if (i % 2 == 1) {
                result *= current_product;
            }
            current_product *= current_product;
        }
        return result;
    };

    std::cout << x << "^" << n << " = " << myPow(x, n) << '\n';
}

// ============================================================
// Q33: Integer to Roman
// ============================================================
// Greedy subtraction from largest symbol value — works because
// Roman numeral system is canonical (subtractive pairs like CM, IV).

void question33() {
    std::cout << "Enter an integer to convert to Roman numeral: ";
    int num;
    std::cin >> num;
    if (num <= 0 || num > 3999) {
        std::cout << "Invalid input: num must be in range [1, 3999].\n";
        return;
    }
    std::vector<std::pair<int, std::string>> value_symbols = {
        {1000, "M"}, {900, "CM"}, {500, "D"}, {400, "CD"},
        {100, "C"}, {90, "XC"}, {50, "L"}, {40, "XL"},
        {10, "X"}, {9, "IX"}, {5, "V"}, {4, "IV"},
        {1, "I"}
    };
    std::string result;
    for (const auto &[value, symbol] : value_symbols) {
        while (num >= value) {
            result += symbol;
            num -= value;
        }
    }
    std::cout << "Roman numeral: " << result << '\n';
}

// ============================================================
// Q34: Roman to Integer
// ============================================================
// Left-to-right scan with one-character lookahead — subtractive notation
// (IV = 5-1) means if next value is larger, subtract current instead of add.

void question34() {
    std::cout << "Enter a Roman numeral to convert to integer: ";
    std::string s;
    std::cin >> s;
    std::unordered_map<char, int> symbol_values = {
        {'I', 1}, {'V', 5}, {'X', 10},
        {'L', 50}, {'C', 100}, {'D', 500},
        {'M', 1000}
    };
    int total = 0;
    for (size_t i = 0; i < s.size(); ++i) {
        int value = symbol_values[s[i]];
        if (i + 1 < s.size() && symbol_values[s[i + 1]] > value) {
            total -= value;  // Subtractive case
        } else {
            total += value;  // Additive case
        }
    }
    std::cout << "Integer value: " << total << '\n';
}

// ============================================================
// Q35: Reverse Bits
// ============================================================
// 32 iterations guaranteed — result is always a valid uint32_t.
// No overflow possible: just rearranging bits via shift and OR.

void question35() {
    std::cout << "Enter a 32-bit unsigned integer to reverse its bits: ";
    uint32_t n;
    std::cin >> n;
    uint32_t result = 0;
    for (int i = 0; i < 32; ++i) {
        result <<= 1;       // Shift result left to make room for next bit
        result |= (n & 1);  // Add least significant bit of n to result
        n >>= 1;            // Shift n right to process the next bit
    }
    std::cout << "Reversed bits: " << result << '\n';
}

// ============================================================
// Q36: Number of 1 Bits (Brian Kernighan's)
// ============================================================
// Same technique as Q2 but with uint32_t for guaranteed 32-bit width.
// O(k) where k = number of set bits (bounded by 32, so effectively O(1)).

void question36() {
    std::cout << "Enter a 32-bit unsigned integer to count its 1 bits: ";
    uint32_t n;
    std::cin >> n;
    int count = 0;
    while (n) {
        n &= (n - 1);  // Clear the least significant set bit
        ++count;
    }
    std::cout << "Number of 1 bits: " << count << '\n';
}

// ============================================================
// Q37: Circular Buffer (Fixed Size Queue)
// ============================================================
// Free-list stack with modulo arithmetic — O(1) push/pop/peek.
// std::optional for pop/peek avoids exceptions on empty buffer.

void question37() {
    std::cout << "Enter buffer capacity: ";
    int cap;
    std::cin >> cap;
    if (cap <= 0) {
        std::cout << "Invalid input: capacity must be > 0.\n";
        return;
    }

    struct CircularBuffer {
        std::vector<int> buffer;
        int head = 0, tail = 0, count = 0, capacity;

        CircularBuffer(int cap) : buffer(cap), capacity(cap) {}

        bool push(int value) {
            if (count == capacity) return false;
            buffer[tail] = value;
            tail = (tail + 1) % capacity;
            ++count;
            return true;
        }

        std::optional<int> pop() {
            if (count == 0) return std::nullopt;
            int value = buffer[head];
            head = (head + 1) % capacity;
            --count;
            return value;
        }

        std::optional<int> peek() const {
            if (count == 0) return std::nullopt;
            return buffer[head];
        }

        bool isFull() const { return count == capacity; }
        bool isEmpty() const { return count == 0; }
        int size() const { return count; }
    };

    CircularBuffer cb(cap);
    std::cout << "Enter commands (push <val>, pop, peek, quit):\n";
    std::string cmd;
    while (std::cin >> cmd) {
        if (cmd == "quit") break;
        if (cmd == "push") {
            int val;
            std::cin >> val;
            if (cb.push(val)) {
                std::cout << "Pushed " << val << " (size=" << cb.size() << ")\n";
            } else {
                std::cout << "Buffer full, cannot push " << val << '\n';
            }
        } else if (cmd == "pop") {
            auto val = cb.pop();
            if (val) {
                std::cout << "Popped " << *val << " (size=" << cb.size() << ")\n";
            } else {
                std::cout << "Buffer empty, cannot pop\n";
            }
        } else if (cmd == "peek") {
            auto val = cb.peek();
            if (val) {
                std::cout << "Front: " << *val << '\n';
            } else {
                std::cout << "Buffer empty\n";
            }
        }
    }
}

// ============================================================
// Q38: LRU Cache (Simple Version)
// ============================================================
// std::list + std::unordered_map for O(1) get/put.
// list maintains access order (front = most recent, back = LRU).
// map gives O(1) lookup; list::splice moves nodes in O(1) without copy.

void question38() {
    std::cout << "Enter LRU cache capacity: ";
    int cap;
    std::cin >> cap;
    if (cap <= 0) {
        std::cout << "Invalid input: capacity must be > 0.\n";
        return;
    }

    struct LRUCache {
        int capacity;
        std::list<std::pair<int, int>> items;  // (key, value), front = MRU
        std::unordered_map<int, std::list<std::pair<int, int>>::iterator> map;

        LRUCache(int cap) : capacity(cap) {}

        int get(int key) {
            auto it = map.find(key);
            if (it == map.end()) return -1;
            items.splice(items.begin(), items, it->second);
            return it->second->second;
        }

        void put(int key, int value) {
            auto it = map.find(key);
            if (it != map.end()) {
                it->second->second = value;
                items.splice(items.begin(), items, it->second);
            } else {
                items.emplace_front(key, value);
                map[key] = items.begin();
                if (static_cast<int>(items.size()) > capacity) {
                    map.erase(items.back().first);
                    items.pop_back();
                }
            }
        }
    };

    LRUCache cache(cap);
    std::cout << "Enter commands (get <key>, put <key> <val>, quit):\n";
    std::string cmd;
    while (std::cin >> cmd) {
        if (cmd == "quit") break;
        if (cmd == "get") {
            int key;
            std::cin >> key;
            int val = cache.get(key);
            if (val == -1) std::cout << "Key " << key << " not found\n";
            else std::cout << "Get " << key << " = " << val << '\n';
        } else if (cmd == "put") {
            int key, val;
            std::cin >> key >> val;
            cache.put(key, val);
            std::cout << "Put " << key << " = " << val << '\n';
        }
    }
}

// ============================================================
// Q39: Memory Pool Allocator
// ============================================================
// Free-list based fixed-size allocator: O(1) alloc/dealloc.
// Pre-allocates contiguous pool, tracks free blocks via pointer stack.
// Note: uses alignas(max_align_t) for portability — real HFT pools
// would accept alignment as a parameter.

void question39() {
    std::cout << "Enter block size and pool capacity: ";
    size_t block_size, pool_capacity;
    std::cin >> block_size >> pool_capacity;
    if (block_size == 0 || pool_capacity == 0) {
        std::cout << "Invalid input: block size and pool capacity must be > 0.\n";
        return;
    }

    struct MemoryPool {
        std::vector<std::aligned_storage_t<1, alignof(std::max_align_t)>> pool;
        std::vector<void*> free_list;
        size_t block_size;
        static constexpr size_t aligned_slots(size_t bsize) {
            return (bsize + alignof(std::max_align_t) - 1) / alignof(std::max_align_t);
        }

        MemoryPool(size_t bsize, size_t capacity)
            : pool(capacity * aligned_slots(bsize)), block_size(bsize) {
            for (size_t i = 0; i < capacity; ++i) {
                free_list.push_back(pool.data() + i * aligned_slots(bsize));
            }
        }

        void* allocate() {
            if (free_list.empty()) return nullptr;
            void* block = free_list.back();
            free_list.pop_back();
            return block;
        }

        bool deallocate(void* block) {
            if (!owns(block)) return false;  // Reject wild pointers
            free_list.push_back(block);
            return true;
        }

        bool owns(void* block) const {
            auto start = static_cast<const char*>(static_cast<const void*>(pool.data()));
            auto end = start + pool.size() * sizeof(std::aligned_storage_t<1, alignof(std::max_align_t)>);
            auto ptr = static_cast<const char*>(block);
            return ptr >= start && ptr < end;
        }
    };

    MemoryPool mem_pool(block_size, pool_capacity);
    std::cout << "Memory pool created with block size " << block_size
              << " and capacity " << pool_capacity << ".\n";
    std::cout << "Enter commands (alloc, free <ptr>, quit):\n";
    std::string cmd;
    std::unordered_map<std::string, void*> allocated_blocks;
    while (std::cin >> cmd) {
        if (cmd == "quit") break;
        if (cmd == "alloc") {
            void* block = mem_pool.allocate();
            if (block) {
                std::string ptr_name = "ptr" + std::to_string(allocated_blocks.size());
                allocated_blocks[ptr_name] = block;
                std::cout << "Allocated " << ptr_name << " at " << block << '\n';
            } else {
                std::cout << "Memory pool exhausted, cannot allocate\n";
            }
        } else if (cmd == "free") {
            std::string ptr_name;
            std::cin >> ptr_name;
            auto it = allocated_blocks.find(ptr_name);
            if (it != allocated_blocks.end()) {
                mem_pool.deallocate(it->second);
                allocated_blocks.erase(it);
                std::cout << "Deallocated " << ptr_name << '\n';
            } else {
                std::cout << "Pointer " << ptr_name << " not found\n";
            }
        }
    }
}

// ============================================================
// Q40: Fast Input Parser
// ============================================================
// Custom getchar-based integer parser — skips cin/scanf overhead.
// Handles negative numbers and arbitrary whitespace.
// For 10^6 integers this is ~5-10x faster than cin >> int.

void question40() {
    std::cout << "Enter the number of integers followed by the integers themselves:\n";
    int n;
    std::cin >> n;
    if (n <= 0) {
        std::cout << "Invalid input: n must be > 0.\n";
        return;
    }

    // Skip remaining whitespace after reading n
    std::cin.ignore();

    // Read raw input via getchar — bypass cin per-integer overhead
    auto read_int = []() -> long long {
        long long x = 0;
        bool neg = false;
        int c;
        // Skip whitespace
        while ((c = getchar()) != EOF && (c == ' ' || c == '\n' || c == '\r' || c == '\t'))
            ;
        if (c == '-') { neg = true; c = getchar(); }
        while (c != EOF && c >= '0' && c <= '9') {
            x = x * 10 + (c - '0');
            c = getchar();
        }
        return neg ? -x : x;
    };

    long long sum = 0;
    for (int i = 0; i < n; ++i) {
        sum += read_int();
    }

    std::cout << "Sum: " << sum << '\n';
}

int main() {
    std::ios::sync_with_stdio(false);
    std::cin.tie(nullptr);
    // Call the desired question function here, e.g.:
    // question1();

    return 0;
}
