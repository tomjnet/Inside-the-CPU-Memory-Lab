// Build a Simple Memory Allocator: Understanding malloc and new - slide 6: allocate: first fit
// Build: make 06_first_fit
#include <cstddef>
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

static long long offset_of(void* p) {         // distance from the start of the arena, -1 for nullptr
    return p ? static_cast<long long>(static_cast<unsigned char*>(p) - arena) : -1;
}

int main() {
    init();
    void* a = allocate(100);
    void* b = allocate(200);
    void* c = allocate(0);                    // like malloc(0) here: a real block of 16 bytes
    void* d = allocate(5000);                 // bigger than the arena
    std::cout << "a = allocate(100)  -> arena + " << offset_of(a) << "\n";
    std::cout << "b = allocate(200)  -> arena + " << offset_of(b) << "   (16 + 112 + 16)\n";
    std::cout << "c = allocate(0)    -> arena + " << offset_of(c) << "   (144 + 208 + 16)\n";
    std::cout << "d = allocate(5000) -> " << (d ? "a block?" : "nullptr: nothing big enough") << "\n\n";

    std::cout << "the walk first fit does, header by header:\n";
    int visited = 0;
    for (Block* blk = first(); blk; blk = next(blk)) {
        std::cout << "  block " << visited++ << ": " << (blk->free ? "free" : "USED") << " " << blk->size << "\n";
    }
    std::cout << "a fourth allocate reads " << visited << " headers before it finds the free block: O(blocks)\n";

    bool ok = offset_of(a) == 16 && offset_of(b) == 144 && offset_of(c) == 368 && d == nullptr && visited == 4;
    std::cout << (ok ? "first fit returned the expected addresses\n" : "UNEXPECTED ADDRESSES\n");
    return ok ? 0 : 1;
}
