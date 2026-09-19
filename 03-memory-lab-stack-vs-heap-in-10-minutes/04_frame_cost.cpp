// Stack vs Heap: Where Does Your Data Actually Live? - slide 4: a stack allocation is one subtract
// Build: make 04_frame_cost
#include <cstdint>
#include <iostream>

struct Point { int x; int y; };

int sum_three(int a, int b, int c) {
    int   total = a + b + c;       // 4 B, or just a register
    char  buf[64];                 // 64 B in the same frame
    Point p{a, b};                 // 8 B in the same frame
    // prologue: sub rsp, N    one subtract reserves all of it
    // epilogue: add rsp, N    one add releases all of it
    // no search, no lock, no system call, no header: O(1)
    buf[0] = static_cast<char>(total + p.x);
    return buf[0] + p.y;
}

// The same three locals, but their addresses are printed, so the compiler must keep them in memory.
// They all sit inside one frame: the distance between the lowest and the highest is about their total size.
static void show_frame(int a, int b, int c) {
    int   total = a + b + c;
    char  buf[64];
    Point p{a, b};
    buf[0] = static_cast<char>(total + p.x);

    auto at = [](const void* q) { return reinterpret_cast<std::uintptr_t>(q); };
    std::uintptr_t lo = at(&total);
    std::uintptr_t hi = at(&total) + sizeof total;
    if (at(buf) < lo) lo = at(buf);
    if (at(&p) < lo) lo = at(&p);
    if (at(buf) + sizeof buf > hi) hi = at(buf) + sizeof buf;
    if (at(&p) + sizeof p > hi) hi = at(&p) + sizeof p;

    std::cout << "  &total " << &total << "  (" << sizeof total << " B)\n";
    std::cout << "  buf    " << static_cast<const void*>(buf) << "  (" << sizeof buf << " B, buf[0] = "
              << static_cast<int>(buf[0]) << ")\n";
    std::cout << "  &p     " << &p << "  (" << sizeof p << " B, p.y = " << p.y << ")\n";
    std::cout << "  sum of the sizes: " << sizeof total + sizeof buf + sizeof p << " B\n";
    std::cout << "  span in the frame: " << hi - lo << " B (sizes plus alignment padding)\n";
}

int main() {
    std::cout << "sum_three(1, 2, 3) = " << sum_three(1, 2, 3) << "\n\n";

    std::cout << "three locals, one frame, one subtract on rsp:\n";
    show_frame(1, 2, 3);

    std::cout << "\nNo allocator ran: the frame was reserved by moving the stack pointer once.\n";
    std::cout << "See it yourself: g++ -O2 -S -masm=intel 04_frame_cost.cpp -o - and look for sub rsp.\n";
    return 0;
}
