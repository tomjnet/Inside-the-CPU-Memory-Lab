// Memory Addresses and Pointers: What Is Really Stored? - slide 7: pointer arithmetic: walk an array
// Build: make 07_pointer_arithmetic
#include <cstdint>
#include <iostream>

int main() {
    int a[4] = {10, 20, 30, 40};
    for (int* p = a; p != a + 4; ++p) {         // ++p adds sizeof(int)
        std::cout << p << ": " << *p << '\n';   // addresses 4 apart
    }
    int* first = a;
    int* last = a + 3;                     // 12 bytes past a
    std::cout << last - first << '\n';     // 3 elements, not 12 bytes
    // one 64 byte cache line holds 16 of these ints

    // the same distance as plain numbers: the byte difference is the element difference times sizeof(int)
    const auto lo = reinterpret_cast<std::uintptr_t>(first);
    const auto hi = reinterpret_cast<std::uintptr_t>(last);
    std::cout << "\nbytes between first and last: " << (hi - lo) << " = 3 * sizeof(int)\n";

    // the step follows the type: the same walk over doubles moves 8 bytes at a time
    double d[3] = {1.5, 2.5, 3.5};
    for (const double* q = d; q != d + 3; ++q) std::cout << q << ": " << *q << "\n";
    const auto step = reinterpret_cast<std::uintptr_t>(d + 1) - reinterpret_cast<std::uintptr_t>(d);
    std::cout << "one step of a double*: " << step << " bytes\n";
    std::cout << "ints per 64 byte cache line: " << 64 / sizeof(int) << "\n";
    return (hi - lo == 3 * sizeof(int) && step == sizeof(double)) ? 0 : 1;
}
