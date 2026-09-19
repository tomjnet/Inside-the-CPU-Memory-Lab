// Memory Addresses and Pointers: What Is Really Stored? - slide 4: take an address, follow it
// Build: make 04_address_and_deref
#include <iostream>

int main() {
    int x = 42;
    int* p = &x;                // & takes the address: no memory read
    std::cout << p << '\n';     // the address: 0x7ffd1c2a4b3c or similar
    std::cout << *p << '\n';    // * follows it: one 4 byte read, 42
    *p = 43;                    // one 4 byte write through the pointer
    std::cout << x << '\n';     // 43: x and *p are the same bytes
    int** pp = &p;              // a pointer has an address too
    std::cout << **pp << '\n';  // two reads: first p, then x

    // the same facts, labelled: the pointer is a variable with its own address and size
    std::cout << "\n&x  (where x lives)    = " << static_cast<const void*>(&x) << "\n";
    std::cout << "p   (the number in p)  = " << static_cast<const void*>(p) << "\n";
    std::cout << "&p  (where p lives)    = " << static_cast<const void*>(&p) << "\n";
    std::cout << "pp  (the number in pp) = " << static_cast<const void*>(pp) << "\n";
    std::cout << "sizeof x = " << sizeof x << ", sizeof p = " << sizeof p << ", sizeof pp = " << sizeof pp << "\n";
    std::cout << "p == &x: " << (p == &x) << ", *pp == p: " << (*pp == p) << "\n";
    std::cout << "the addresses change on every run: the operating system randomizes the layout\n";
    return (p == &x && *pp == p && x == 43) ? 0 : 1;
}
