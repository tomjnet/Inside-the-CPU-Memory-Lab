// Virtual Memory in 10 Minutes: How Every Process Gets Its Own Memory - slide 6: addresses by region
// Build: make 06_regions
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <memory>

// Prints one virtual address. A function pointer cannot become a void*, but any pointer can become an integer.
template <class T>
static void show(const char* region, T* p) {
    std::cout << "  " << region << "  0x" << std::hex << std::setw(12) << std::setfill('0')
              << reinterpret_cast<std::uintptr_t>(p) << std::dec << std::setfill(' ') << '\n';
}

int global_counter = 1;                     // data: lives all run long

void print_regions() {
    int local = 2;                          // stack: top of the space
    auto heap = std::make_unique<int>(3);   // heap: above the data
    show("text ", &print_regions);          // code: read only pages
    show("data ", &global_counter);
    show("heap ", heap.get());
    show("stack", &local);                  // every one is virtual:
}                                           // no RAM position in sight

// The distance between two addresses, in the unit that reads best.
static void gap(const char* label, std::uintptr_t a, std::uintptr_t b) {
    const double bytes = static_cast<double>(a > b ? a - b : b - a);
    const double kib = 1024.0, mib = kib * 1024.0, gib = mib * 1024.0;
    std::cout << "  " << label << "  " << std::fixed << std::setprecision(1);
    if (bytes >= gib) std::cout << bytes / gib << " GiB\n";
    else if (bytes >= mib) std::cout << bytes / mib << " MiB\n";
    else std::cout << bytes / kib << " KiB\n";
}

int main() {
    std::cout << "one object per region, virtual addresses:\n";
    print_regions();

    int local = 0;
    auto heap = std::make_unique<int>(0);
    const auto text_at = reinterpret_cast<std::uintptr_t>(&print_regions);
    const auto data_at = reinterpret_cast<std::uintptr_t>(&global_counter);
    const auto heap_at = reinterpret_cast<std::uintptr_t>(heap.get());
    const auto stack_at = reinterpret_cast<std::uintptr_t>(&local);

    std::cout << "\ngaps in this run:\n";
    gap("text  to data ", text_at, data_at);
    gap("data  to heap ", data_at, heap_at);
    gap("heap  to stack", heap_at, stack_at);

#if defined(__linux__)
    std::cout << "\nLinux order, low to high: text, data, heap, (mmap region), stack: "
              << ((text_at < data_at && data_at < heap_at && heap_at < stack_at) ? "yes" : "not in this build") << '\n';
#else
    std::cout << "\nthis is not Linux: the regions exist too, but the order and the gaps differ\n";
#endif
    std::cout << "run it again: address space layout randomization moves every region\n";
    return 0;
}
