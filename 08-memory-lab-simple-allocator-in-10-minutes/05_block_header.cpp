// Build a Simple Memory Allocator: Understanding malloc and new - slide 5: a header in front of every block
// Build: make 05_block_header
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

static std::size_t offset_of(void* p) {       // distance from the start of the arena, easier to read than an address
    return static_cast<std::size_t>(static_cast<unsigned char*>(p) - arena);
}

int main() {
    static_assert(sizeof(Block) <= ALIGN, "the header must fit in 16 bytes");
    std::cout << "sizeof(Block) = " << sizeof(Block) << ", alignof(Block) = " << alignof(Block)
              << " (size_t + bool + padding)\n";

    init();
    Block* b = first();
    std::cout << "after init(): header at arena + " << offset_of(b) << ", payload at arena + "
              << offset_of(payload(b)) << ", size " << b->size << ", free " << b->free << "\n";
    bool ok = offset_of(payload(b)) == ALIGN && b->size == ARENA - ALIGN && next(b) == nullptr;
    std::cout << "next(first()) = nullptr: one block covers the whole arena\n\n";

    split(b, 112);                            // what allocate(100) will do on the next slide
    Block* rest = next(b);
    std::cout << "after split(first(), 112):\n";
    std::cout << "  block 0: header at arena + " << offset_of(b) << ", size " << b->size << "\n";
    std::cout << "  block 1: header at arena + " << offset_of(rest) << ", size " << rest->size << "\n";
    std::cout << "  header + 16 + size = next header: 0 + 16 + 112 = " << offset_of(rest) << "\n";
    ok = ok && offset_of(rest) == 128 && rest->size == 3952 && rest->free && next(rest) == nullptr;

    // the way back, which deallocate uses: payload pointer minus 16 is the header
    void* user = payload(rest);
    auto* back = reinterpret_cast<Block*>(static_cast<unsigned char*>(user) - ALIGN);
    std::cout << "user pointer arena + " << offset_of(user) << " minus 16 -> header with size " << back->size << "\n";
    ok = ok && back == rest;
    std::cout << (ok ? "header arithmetic checks out\n" : "HEADER ARITHMETIC IS WRONG\n");
    return ok ? 0 : 1;
}
