// Pages and Page Tables: How Virtual Addresses Reach RAM - slide 2: the page: the unit of every mapping
// Build: make 02_page_and_offset
#include <cstdint>
#include <iostream>

int main() {
    // a page is 4 KiB: 2 to the 12 bytes, the unit of every mapping
    constexpr std::uint64_t kPage = 4096;
    std::uint64_t addr   = 0x7ffd1234abcd;
    std::uint64_t page   = addr >> 12;          // virtual page number
    std::uint64_t offset = addr & (kPage - 1);  // 0xbcd: never translated
    // one entry per page, not per byte: 1 GiB needs 262144 entries
    std::uint64_t entries = (1ull << 30) / kPage;
    // per byte it would be 2 to the 30 entries of 8 bytes each:
    // 8 GiB of tables to map 1 GiB of memory

    std::cout << std::hex << "address              0x" << addr << "\n"
              << "virtual page number  0x" << page << "   (address >> 12: this part is translated)\n"
              << "offset in the page   0x" << offset << "         (address & 0xfff: copied as it is)\n"
              << "page start           0x" << (page << 12) << "\n" << std::dec;

    // every byte of one page shares the page number: one entry serves all 4096 of them
    std::uint64_t first = page << 12;
    std::uint64_t last = first + kPage - 1;
    std::cout << "\nfirst and last byte of that page: page number 0x" << std::hex << (first >> 12) << " and 0x"
              << (last >> 12) << std::dec << "\n";
    if ((first >> 12) != (last >> 12)) {
        std::cout << "error: one page, two page numbers\n";
        return 1;
    }

    const std::uint64_t gib = 1ull << 30;
    std::uint64_t per_page_bytes = entries * 8;          // last level entries only
    std::uint64_t per_byte_bytes = gib * 8;              // the loose claim, taken literally
    std::cout << "\nmapping 1 GiB\n"
              << "  one entry per page: " << entries << " entries, " << per_page_bytes / (1u << 20) << " MiB of tables\n"
              << "  one entry per byte: " << gib << " entries, " << per_byte_bytes / gib << " GiB of tables\n"
              << "a page table entry maps a page, not a byte\n";
    return 0;
}
