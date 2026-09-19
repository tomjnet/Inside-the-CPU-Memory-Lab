// Build a Simple Memory Allocator: Understanding malloc and new - slide 11: fragmentation
// Build: make 11_fragmentation
#include <cstddef>
#include <iomanip>
#include <iostream>
#include <new>

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

static std::size_t free_total() {
    std::size_t total = 0;
    for (Block* b = first(); b; b = next(b)) if (b->free) total += b->size;
    return total;
}
static std::size_t largest_hole() {
    std::size_t best = 0;
    for (Block* b = first(); b; b = next(b)) if (b->free && b->size > best) best = b->size;
    return best;
}

int main() {
    init();
    void* p[8];
    for (auto& q : p) q = allocate(480);         // 8 x (16 + 480) = 3968
    for (int i = 0; i < 8; i += 2) deallocate(p[i]);   // every other one
    print_map("4 holes");                        // free total: 2032 bytes
    void* big = allocate(1024);                  // nullptr: best hole 480
    std::cout << "free " << free_total() << " bytes, largest hole " << largest_hole()
              << ", allocate(1024) -> " << (big ? "a block" : "nullptr") << "\n\n";
    bool ok = big == nullptr && free_total() == 2032 && largest_hole() == 480;

    deallocate(p[1]);                            // 0, 1, 2 merge: 1472
    big = allocate(1024);                        // now it fits
    print_map("p[1] freed, 1024");
    std::cout << "holes 0, 1 and 2 merged into 480 + 16 + 480 + 16 + 480 = 1472, allocate(1024) -> "
              << (big ? "a block" : "nullptr") << "\n";
    std::cout << "same free bytes as before, but now in one piece: a block must be contiguous\n";
    ok = ok && big != nullptr && big == p[0];             // first fit: the merged hole starts where p[0] was
    return ok ? 0 : 1;
}
