// Memory Addresses and Pointers: What Is Really Stored? - slide 5: sizeof(t*): every pointer is 8 bytes
// Build: make 05_pointer_sizes
#include <iostream>

struct Big { char bytes[4096]; };

int main() {
    // every object pointer is one 64 bit address on x86 64
    std::cout << sizeof(char*) << '\n';     // 8
    std::cout << sizeof(int*) << '\n';      // 8
    std::cout << sizeof(double*) << '\n';   // 8
    std::cout << sizeof(void*) << '\n';     // 8: an address with no type
    // the type decides how many bytes one * reads
    double d = 1.5;
    double* pd = &d;
    std::cout << sizeof(pd) << ' ' << sizeof(*pd) << '\n';   // 8 8
    char c = 'A';
    char* pc = &c;
    std::cout << sizeof(pc) << ' ' << sizeof(*pc) << '\n';   // 8 1

    // a pointer to a 4096 byte object is still one address
    static Big big{};
    Big* pb = &big;
    std::cout << "\nsizeof(Big*) = " << sizeof(pb) << ", sizeof(Big) = " << sizeof(*pb) << "\n";
    std::cout << "*pd = " << *pd << ", *pc = " << *pc << ", first byte of big = " << int(pb->bytes[0]) << "\n";
    std::cout << "this build: " << sizeof(void*) * 8 << " bit addresses, " << 64 / sizeof(void*)
              << " pointers per 64 byte cache line\n";
    std::cout << "a list node {int, next}: " << sizeof(int) << " bytes of data, " << sizeof(void*)
              << " bytes of pointer\n";
    return 0;
}
