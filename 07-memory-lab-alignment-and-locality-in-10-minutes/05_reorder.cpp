// Memory Alignment and Locality: Why Data Layout Matters - slide 5: reordering members
// Build: make 05_reorder
#include <cstddef>
#include <iostream>

struct Before {        // 24 bytes: 10 of them are padding
    char   tag;        // offset 0, then 7 bytes of padding
    double price;      // offset 8: must be a multiple of 8
    char   side;       // offset 16, then 3 bytes of padding
    int    qty;        // offset 20
};
struct After {         // 16 bytes: same data, 2 bytes of padding
    double price;      // offset 0: biggest alignment first
    int    qty;        // offset 8
    char   tag;        // offset 12
    char   side;       // offset 13, then 2 bytes of tail padding
};

int main() {
    const std::size_t data = sizeof(char) + sizeof(double) + sizeof(char) + sizeof(int);

    std::cout << "struct Before: sizeof " << sizeof(Before) << ", alignof " << alignof(Before) << "\n";
    std::cout << "  tag   at offset " << offsetof(Before, tag) << "\n";
    std::cout << "  price at offset " << offsetof(Before, price) << "\n";
    std::cout << "  side  at offset " << offsetof(Before, side) << "\n";
    std::cout << "  qty   at offset " << offsetof(Before, qty) << "\n";
    std::cout << "  data " << data << " bytes, padding " << sizeof(Before) - data << " bytes\n\n";

    std::cout << "struct After: sizeof " << sizeof(After) << ", alignof " << alignof(After) << "\n";
    std::cout << "  price at offset " << offsetof(After, price) << "\n";
    std::cout << "  qty   at offset " << offsetof(After, qty) << "\n";
    std::cout << "  tag   at offset " << offsetof(After, tag) << "\n";
    std::cout << "  side  at offset " << offsetof(After, side) << "\n";
    std::cout << "  data " << data << " bytes, padding " << sizeof(After) - data << " bytes\n\n";

    // What the difference means for a loop: objects per 64 byte cache line and memory for a million of them.
    const std::size_t line = 64, million = 1000000;
    std::cout << "whole objects per cache line: Before " << line / sizeof(Before) << ", After " << line / sizeof(After) << "\n";
    std::cout << "one million objects: Before " << sizeof(Before) * million / 1000000 << " MB, After "
              << sizeof(After) * million / 1000000 << " MB\n";

    // Same members, same values: only the order of the declaration changed.
    Before b{'A', 101.5, 'B', 300};
    After a{b.price, b.qty, b.tag, b.side};
    std::cout << "same data: " << a.tag << " " << a.price << " " << a.side << " " << a.qty << "\n";
    return sizeof(After) <= sizeof(Before) ? 0 : 1;
}
