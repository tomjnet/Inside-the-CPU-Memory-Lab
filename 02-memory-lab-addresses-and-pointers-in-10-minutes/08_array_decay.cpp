// Memory Addresses and Pointers: What Is Really Stored? - slide 8: arrays decay to pointers
// Build: make 08_array_decay
#include <cstddef>
#include <iostream>
#include <span>

// an array parameter is a pointer: the length is gone
long long sum(const int* p, std::size_t n) {
    long long s = 0;
    for (std::size_t i = 0; i < n; ++i) s += p[i];   // *(p + i)
    return s;
}

// C++20: std::span carries the pointer and the length together
static long long sum_span(std::span<const int> v) {
    long long s = 0;
    for (int x : v) s += x;
    return s;
}

int main() {
    int a[4] = {10, 20, 30, 40};
    std::cout << sizeof a << '\n';      // 16: still an array here
    const int* p = a;                   // decay: the address of a[0]
    std::cout << sizeof p << '\n';      // 8: only the address is left
    std::cout << sum(a, 4) << '\n';     // so pass the length yourself

    std::cout << "\np == &a[0]: " << (p == &a[0]) << "\n";
    std::cout << "a[2] = " << a[2] << ", *(a + 2) = " << *(a + 2) << ", p[2] = " << p[2] << "\n";
    std::cout << "elements from sizeof: " << sizeof a / sizeof a[0] << " (only where a is still an array)\n";
    std::cout << "sum through std::span: " << sum_span(a) << ", sizeof(std::span<const int>) = "
              << sizeof(std::span<const int>) << "\n";
    return (sum(a, 4) == 100 && sum_span(a) == 100) ? 0 : 1;
}
