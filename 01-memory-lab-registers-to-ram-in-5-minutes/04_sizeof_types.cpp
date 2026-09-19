// Computer Memory in 5 Minutes: From Registers to RAM - slide 4: bits, bytes and the address as a number
// Build: make 04_sizeof_types
#include <climits>
#include <cstddef>
#include <cstdint>
#include <iostream>

int main() {
    // sizeof is answered by the compiler: zero cost at run time
    std::cout << "bits in a byte " << CHAR_BIT << "\n";          // 8
    std::cout << "char      " << sizeof(char) << "\n";           // 1
    std::cout << "short     " << sizeof(short) << "\n";          // 2
    std::cout << "int       " << sizeof(int) << "\n";            // 4
    std::cout << "long      " << sizeof(long) << "\n";   // 8, Windows 4
    std::cout << "long long " << sizeof(long long) << "\n";      // 8
    std::cout << "double    " << sizeof(double) << "\n";         // 8
    // 8 bytes = 64 bits: a pointer holds an address, a plain number
    std::cout << "void*     " << sizeof(void*) << "\n";          // 8

    // The compiler really answers it: these are checked before the program exists.
    static_assert(sizeof(char) == 1, "a char is one byte by definition");
    static_assert(sizeof(void*) * CHAR_BIT == 64, "this lab assumes a 64 bit build");

    // Memory is a row of bytes and the address is the index: four neighbours, four consecutive numbers.
    static unsigned char row[4] = {10, 20, 30, 40};
    std::cout << "\nfour bytes in a row (this machine, this run):\n";
    for (std::size_t i = 0; i < 4; ++i) {
        auto address = reinterpret_cast<std::uintptr_t>(&row[i]);
        std::cout << "  row[" << i << "] = " << static_cast<int>(row[i]) << "  at " << static_cast<const void*>(&row[i])
                  << "  = " << address << " as a plain number\n";
    }
    auto first = reinterpret_cast<std::uintptr_t>(&row[0]);
    auto last = reinterpret_cast<std::uintptr_t>(&row[3]);
    std::cout << "last minus first = " << (last - first) << " bytes: the address counts bytes\n";
    return (last - first == 3) ? 0 : 1;
}
