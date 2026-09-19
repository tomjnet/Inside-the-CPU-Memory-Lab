// Build a Simple Memory Allocator: Understanding malloc and new - slide 10: check every pointer
// Build: make 10_check_pointers
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <iomanip>
#include <iostream>
#include <new>
#include <random>

// ---- slide 3: the arena and the interface ----
constexpr std::size_t ARENA = 4096;          // one page, fixed forever
constexpr std::size_t ALIGN = 16;            // what malloc promises
alignas(16) static unsigned char arena[ARENA];

// round n up to a multiple of 16: 1 -> 16, 16 -> 16, 17 -> 32
constexpr std::size_t align_up(std::size_t n) {
    return (n + ALIGN - 1) & ~(ALIGN - 1);
}

void* allocate(std::size_t size);            // nullptr when it is full
void  deallocate(void* ptr);                 // no size: we store it

// ---- slide 5: a header in front of every block ----
struct Block {                    // header in front of every payload
    std::size_t size;             // payload bytes, multiple of 16
    bool        free;             // may allocate() hand it out?
};                                // 16 bytes with padding on x86 64
Block* first() { return reinterpret_cast<Block*>(arena); }
unsigned char* payload(Block* b) {           // bytes after the header
    return reinterpret_cast<unsigned char*>(b) + ALIGN;
}
Block* next(Block* b) {                      // header + payload = next
    unsigned char* p = payload(b) + b->size;
    return p < arena + ARENA ? reinterpret_cast<Block*>(p) : nullptr;
}

// ---- slide 7: init and split ----
void init() {                                // one free block: 4080
    new (arena) Block{ARENA - ALIGN, true};
}
void split(Block* b, std::size_t need) {     // O(1): write one header
    if (b->size < need + 2 * ALIGN) return;  // rest too small: keep it
    new (payload(b) + need) Block{b->size - need - ALIGN, true};
    b->size = need;                          // b shrinks, rest is free
}

// ---- slide 6: allocate: first fit ----
void* allocate(std::size_t size) {           // O(blocks): walk them all
    std::size_t need = align_up(size ? size : 1);
    for (Block* b = first(); b; b = next(b)) {
        if (!b->free || b->size < need) continue;   // first fit
        split(b, need);                      // give back what is left
        b->free = false;
        return payload(b);                   // 16 bytes past the header
    }
    return nullptr;                          // nothing big enough
}

// ---- slide 8: deallocate and coalesce ----
void deallocate(void* ptr) {                 // O(blocks): one pass
    if (!ptr) return;                        // like free(nullptr)
    auto* bytes = static_cast<unsigned char*>(ptr);
    reinterpret_cast<Block*>(bytes - ALIGN)->free = true;   // back 16
    for (Block* b = first(); b; b = next(b))               // coalesce
        while (b->free && next(b) && next(b)->free)
            b->size += ALIGN + next(b)->size;    // absorb the neighbour
}

// ---- slide 9: print the block map ----
void print_map(const char* what) {           // one line per operation
    std::cout << std::left << std::setw(20) << what << '|';
    for (Block* b = first(); b; b = next(b))
        std::cout << (b->free ? " free " : " USED ") << b->size << " |";
    std::cout << '\n';
}
// init()              | free 4080 |
// a = allocate(100)   | USED 112 | free 3952 |
// b = allocate(200)   | USED 112 | USED 208 | free 3728 |
// deallocate(a)       | free 112 | USED 208 | free 3728 |
// deallocate(b)       | free 4080 |

// ---- slide 10: check every pointer ----
bool valid(void* ptr) {                      // what every block obeys
    auto a  = reinterpret_cast<std::uintptr_t>(ptr);
    auto lo = reinterpret_cast<std::uintptr_t>(arena);
    return a % ALIGN == 0                    // aligned to 16
        && a >= lo + ALIGN                   // after the first header
        && a <  lo + ARENA;                  // inside the arena
}
// stress: 1000 random operations, sizes from 1 to 256 bytes:
// every pointer valid, every block keeps its fill byte (no overlap),
// and after freeing everything the map is one free block of 4080

// every payload plus 16 bytes per header must add up to the arena: true after any operation
static bool adds_up() {
    std::size_t total = 0;
    for (Block* b = first(); b; b = next(b)) total += ALIGN + b->size;
    return total == ARENA;
}

struct Live { unsigned char* ptr; std::size_t size; unsigned char fill; };

static bool intact(const Live& l) {           // does the block still hold the byte we wrote into it?
    for (std::size_t i = 0; i < l.size; ++i)
        if (l.ptr[i] != l.fill) return false;
    return true;
}

int main() {
    init();
    static Live live[32] = {};                // at most 32 blocks alive at the same time
    std::mt19937 rng(8);                      // fixed seed: the same run on every machine
    int allocs = 0, frees = 0, full = 0, bad_pointer = 0, overlap = 0, broken = 0;

    for (int op = 0; op < 1000; ++op) {
        Live& slot = live[rng() % 32];
        if (slot.ptr == nullptr) {
            std::size_t size = rng() % 256 + 1;
            void* p = allocate(size);
            if (p == nullptr) { ++full; continue; }        // the arena may be full: that is allowed
            if (!valid(p)) ++bad_pointer;
            slot = Live{static_cast<unsigned char*>(p), size, static_cast<unsigned char>(op % 251 + 1)};
            std::memset(slot.ptr, slot.fill, slot.size);
            ++allocs;
        } else {
            if (!intact(slot)) ++overlap;                  // someone else wrote into our bytes
            deallocate(slot.ptr);
            slot = Live{};
            ++frees;
        }
        if (!adds_up()) ++broken;
    }
    print_map("after 1000 ops");

    for (Live& slot : live) {
        if (slot.ptr == nullptr) continue;
        if (!intact(slot)) ++overlap;
        deallocate(slot.ptr);
        ++frees;
    }
    print_map("all freed");

    bool one_block = first()->free && first()->size == ARENA - ALIGN && next(first()) == nullptr;
    std::cout << "\n" << allocs << " allocations, " << frees << " frees, " << full << " requests refused (arena full)\n";
    std::cout << "pointers not aligned or outside the arena: " << bad_pointer << "\n";
    std::cout << "blocks that lost their fill byte (overlap): " << overlap << "\n";
    std::cout << "operations after which the sizes did not add up: " << broken << "\n";
    std::cout << "one free block of 4080 at the end: " << (one_block ? "yes" : "NO") << "\n";
    bool ok = bad_pointer == 0 && overlap == 0 && broken == 0 && one_block && allocs == frees && !valid(arena);
    return ok ? 0 : 1;
}
