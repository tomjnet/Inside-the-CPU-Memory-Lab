// Memory Alignment and Locality: Why Data Layout Matters - slide 3: alignof in code
// Build: make 03_alignof
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <memory>

static double global_d = 2.71;     // static storage, for the check at the end

template <class T>
static std::uintptr_t remainder_of(const T* p) {
    return reinterpret_cast<std::uintptr_t>(p) % alignof(T);
}

int main() {
    // alignof(T): every address of a T is a multiple of it
    std::cout << alignof(char)   << '\n';             // 1
    std::cout << alignof(short)  << '\n';             // 2
    std::cout << alignof(int)    << '\n';             // 4
    std::cout << alignof(double) << '\n';             // 8 on x86 64
    std::cout << alignof(std::max_align_t) << '\n';   // 16 with g++

    double d = 3.14;                       // check it: the low bits are 0
    auto addr = reinterpret_cast<std::uintptr_t>(&d);
    std::cout << addr % alignof(double) << '\n';      // always 0

    // The same table with names, plus sizeof: for the fundamental types the two numbers match.
    std::cout << "\ntype            sizeof  alignof\n";
    std::cout << "char            " << sizeof(char) << "       " << alignof(char) << "\n";
    std::cout << "short           " << sizeof(short) << "       " << alignof(short) << "\n";
    std::cout << "int             " << sizeof(int) << "       " << alignof(int) << "\n";
    std::cout << "long long       " << sizeof(long long) << "       " << alignof(long long) << "\n";
    std::cout << "double          " << sizeof(double) << "       " << alignof(double) << "\n";
    std::cout << "void*           " << sizeof(void*) << "       " << alignof(void*) << "\n";
    std::cout << "max_align_t     " << sizeof(std::max_align_t) << "      " << alignof(std::max_align_t)
              << "   (MSVC reports 8 here; its 64 bit heap still aligns blocks to 16)\n";

    // The rule holds for every storage region: local, global and heap.
    auto heap_d = std::make_unique<double>(1.41);
    std::uintptr_t bad = remainder_of(&d) + remainder_of(&global_d) + remainder_of(heap_d.get());
    std::cout << "\naddress % alignof(double): local " << remainder_of(&d) << ", global " << remainder_of(&global_d)
              << ", heap " << remainder_of(heap_d.get()) << "\n";
    std::cout << "heap block % 16: " << reinterpret_cast<std::uintptr_t>(heap_d.get()) % 16
              << "   (new and malloc align every block for any fundamental type)\n";
    std::cout << "values: " << d << " " << global_d << " " << *heap_d << "\n";
    return bad == 0 ? 0 : 1;
}
