// Computer Memory in 5 Minutes: From Registers to RAM - slide 5: three addresses: global, local and heap
// Build: make 05_three_addresses
#include <cstdint>
#include <iostream>

int global_counter = 7;            // static storage: lives all run

void show() {
    int local = 1;                 // stack: moves one register
    int* heap = new int(2);        // heap: a call into the allocator
    std::cout << &global_counter << "\n" << &local << "\n"
              << heap << "\n";     // three numbers, three regions
    delete heap;                   // the heap needs a delete
}

// The same three objects again, as plain numbers, so the distances between the regions can be printed.
static double mib_between(std::uintptr_t a, std::uintptr_t b) {
    std::uintptr_t d = a > b ? a - b : b - a;
    return static_cast<double>(d) / (1024.0 * 1024.0);
}

int main() {
    std::cout << "global, local, heap (this machine, this run):\n";
    show();

    int local = 1;
    int* heap = new int(2);
    auto g = reinterpret_cast<std::uintptr_t>(&global_counter);
    auto l = reinterpret_cast<std::uintptr_t>(&local);
    auto h = reinterpret_cast<std::uintptr_t>(heap);
    std::cout << "\nvalues: " << global_counter << " " << local << " " << *heap << "\n";
    std::cout << "global to heap : " << mib_between(g, h) << " MiB apart\n";
    std::cout << "heap to stack  : " << mib_between(h, l) << " MiB apart\n";
    std::cout << "global to stack: " << mib_between(g, l) << " MiB apart\n";
    std::cout << "highest address: " << ((l > g && l > h) ? "the stack (typical on Linux)" : "not the stack on this system")
              << "\n";
    std::cout << "run it twice: the numbers change (address space layout randomization), the regions do not\n";
    delete heap;
    return 0;
}
