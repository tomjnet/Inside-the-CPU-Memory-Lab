// Memory Addresses and Pointers: What Is Really Stored? - slide 10: references: an alias, not a new object
// Build: make 10_references
#include <iostream>

void bump(int& n) { ++n; }   // compiled as an address, used as a name

// the pointer version: the caller writes &, the callee has to test
static void bump_ptr(int* n) {
    if (n) ++*n;
}

struct HoldsRef { int& r; };
struct HoldsPtr { int* p; };

int main() {
    int x = 42;
    int& r = x;                        // an alias of x, not a new object
    std::cout << (&r == &x) << '\n';   // 1: the same address
    r = 43;                            // writes x: no * needed
    int y = 7;
    r = y;                             // copies 7 into x: no reseating
    bump(x);                           // passes the address of x
    std::cout << x << '\n';            // 8
    // never null, never rebinds: prefer a reference to a pointer

    std::cout << "\n&x = " << static_cast<const void*>(&x) << ", &r = " << static_cast<const void*>(&r)
              << ", &y = " << static_cast<const void*>(&y) << "\n";
    std::cout << "y is still " << y << ": r = y copied a value, r did not move\n";
    bump_ptr(&x);
    bump_ptr(nullptr);                 // legal for a pointer, impossible to write for a reference
    std::cout << "after bump_ptr(&x): x = " << x << "\n";
    // stored inside an object, a reference takes the room of an address
    std::cout << "sizeof(HoldsRef) = " << sizeof(HoldsRef) << ", sizeof(HoldsPtr) = " << sizeof(HoldsPtr) << "\n";
    return (x == 9 && y == 7) ? 0 : 1;
}
