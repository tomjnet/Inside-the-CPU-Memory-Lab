// Pages and Page Tables: How Virtual Addresses Reach RAM - slide 7: split an address in c++
// Build: make 07_split_address
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <memory>

// x86 64, 4 level paging: 9 + 9 + 9 + 9 + 12 = 48 bits
constexpr std::uint64_t index(std::uint64_t va, int level) {
    return (va >> (12 + 9 * level)) & 0x1FF;   // 9 bits: 0 to 511
}

static_assert(index(0x00007f3a5c2d1e48, 3) == 254 && index(0x00007f3a5c2d1e48, 0) == 209, "the numbers of the slide");

static int global_value = 7;

static void split(const char* label, std::uint64_t a) {
    std::cout << std::left << std::setw(10) << label << std::right << " 0x" << std::hex << std::setw(16)
              << std::setfill('0') << a << std::setfill(' ') << std::dec << "  PML4 " << std::setw(3) << index(a, 3)
              << "  PDPT " << std::setw(3) << index(a, 2) << "  PD " << std::setw(3) << index(a, 1) << "  PT "
              << std::setw(3) << index(a, 0) << "  offset 0x" << std::hex << (a & 0xFFF) << std::dec << "\n";
}

int main() {
    std::uint64_t va = 0x00007f3a5c2d1e48;
    auto pml4 = index(va, 3);            // bits 47 to 39: 254
    auto pdpt = index(va, 2);            // bits 38 to 30: 233
    auto pd   = index(va, 1);            // bits 29 to 21: 225
    auto pt   = index(va, 0);            // bits 20 to 12: 209
    std::uint64_t offset = va & 0xFFF;   // bits 11 to 0: 0xe48
    // bits 63 to 48 copy bit 47: the canonical form

    std::cout << "the address of the slide\n";
    split("example", va);

    // put the fields back together: nothing was lost, the split is only shifts and masks
    std::uint64_t again = (pml4 << 39) | (pdpt << 30) | (pd << 21) | (pt << 12) | offset;
    std::cout << "rebuilt    0x" << std::hex << std::setw(16) << std::setfill('0') << again << std::setfill(' ')
              << std::dec << (again == va ? "  (same address)" : "  (MISMATCH)") << "\n";
    if (again != va) return 1;

    int local_value = 1;
    auto heap_value = std::make_unique<int>(2);
    std::cout << "\nyour own addresses (they change on every run: the kernel randomizes the layout)\n";
    split("local", reinterpret_cast<std::uintptr_t>(&local_value));
    split("next int", reinterpret_cast<std::uintptr_t>(&local_value) + sizeof(int));
    split("global", reinterpret_cast<std::uintptr_t>(&global_value));
    split("heap", reinterpret_cast<std::uintptr_t>(heap_value.get()));
    std::cout << "\nlocal and the int after it share the four indexes (unless they straddle a page): only the offset\n"
              << "moves. Stack, globals and heap sit far apart, so they usually differ at the PML4 or PDPT index.\n"
              << "values: " << local_value + global_value + *heap_value << "\n";
    return 0;
}
