// Pages and Page Tables: How Virtual Addresses Reach RAM - slide 8: a page walk as a model
// Build: make 08_walk_model
#include <cstdint>
#include <iostream>
#include <memory>
#include <vector>

struct Table;
struct Entry {
    bool present = false;
    std::uint64_t frame = 0;        // used by the last level
    Table* next = nullptr;          // used by the three upper levels
};
struct Table { Entry e[512]; };
struct PageFault { std::uint64_t va; };

constexpr std::uint64_t index(std::uint64_t va, int level) {
    return (va >> (12 + 9 * level)) & 0x1FF;
}

// four table reads per translation: that is what the TLB saves
std::uint64_t translate(const Table* pml4, std::uint64_t va) {
    const Table* t = pml4;                        // CR3 points here
    for (int level = 3; level > 0; --level) {     // PML4, PDPT, PD
        const Entry& e = t->e[index(va, level)];  // one memory read
        if (!e.present) throw PageFault{va};      // no mapping
        t = e.next;
    }
    const Entry& pte = t->e[index(va, 0)];        // PT: the last read
    if (!pte.present) throw PageFault{va};
    return (pte.frame << 12) | (va & 0xFFF);      // frame + offset
}

// The kernel side of the model: create the missing tables on the way down, then fill the last entry.
static std::vector<std::unique_ptr<Table>> g_tables;

static Table* new_table() {
    g_tables.push_back(std::make_unique<Table>());
    return g_tables.back().get();
}

static void map_page(Table* pml4, std::uint64_t va, std::uint64_t frame) {
    Table* t = pml4;
    for (int level = 3; level > 0; --level) {
        Entry& e = t->e[index(va, level)];
        if (!e.present) { e.present = true; e.next = new_table(); }
        t = e.next;
    }
    Entry& pte = t->e[index(va, 0)];
    pte.present = true;
    pte.frame = frame;
}

// The same walk as translate(), printing every read.
static void trace(const Table* pml4, std::uint64_t va) {
    static const char* names[] = {"PT  ", "PD  ", "PDPT", "PML4"};
    const Table* t = pml4;
    std::cout << std::hex << "walk 0x" << va << std::dec << "\n";
    for (int level = 3; level >= 0; --level) {
        const Entry& e = t->e[index(va, level)];
        std::cout << "  read " << 4 - level << ": " << names[level] << "[" << index(va, level) << "] "
                  << (e.present ? (level ? "-> next table" : "-> frame") : "not present: PAGE FAULT") << "\n";
        if (!e.present) return;
        if (level) t = e.next;
        else std::cout << std::hex << "  physical 0x" << ((e.frame << 12) | (va & 0xFFF)) << std::dec << "\n";
    }
}

int main() {
    Table* pml4 = new_table();
    map_page(pml4, 0x00007f3a5c2d1000, 0x1a2b3);     // the page of the address of the slide
    map_page(pml4, 0x00007f3a5c2d2000, 0x0c411);     // its neighbour: same PT, next entry
    map_page(pml4, 0x0000000000400000, 0x7e090);     // far away: needs its own PDPT, PD and PT
    std::cout << "3 pages mapped with " << g_tables.size() << " tables of 512 entries: the tree only grows where "
              << "something is mapped\n(one flat table for 48 bits would need 2 to the 36 entries)\n\n";

    trace(pml4, 0x00007f3a5c2d1e48);
    trace(pml4, 0x00007f3a5c2d1008);                 // same page: the same four reads, only the offset differs
    trace(pml4, 0x00007f3a5c2d2010);
    trace(pml4, 0x00007f3a5c2d3000);                 // not mapped

    bool ok = translate(pml4, 0x00007f3a5c2d1e48) == 0x1a2b3e48 && translate(pml4, 0x00007f3a5c2d2010) == 0x0c411010
           && translate(pml4, 0x0000000000400abc) == 0x7e090abc;
    bool faulted = false;
    try {
        std::uint64_t pa = translate(pml4, 0x00007f3a5c2d3000);
        std::cout << "unexpected translation 0x" << std::hex << pa << std::dec << "\n";
    } catch (const PageFault& f) {
        faulted = true;
        std::cout << std::hex << "\ntranslate threw PageFault for 0x" << f.va << std::dec
                  << ": the real CPU calls the kernel here\n";
    }
    std::cout << "translations " << (ok ? "correct" : "WRONG") << ", unmapped address "
              << (faulted ? "faulted" : "DID NOT FAULT") << "\n";
    return ok && faulted ? 0 : 1;
}
