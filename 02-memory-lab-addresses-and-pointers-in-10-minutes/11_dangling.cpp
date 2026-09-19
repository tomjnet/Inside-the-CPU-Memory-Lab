// Memory Addresses and Pointers: What Is Really Stored? - slide 11: dangling pointers: the address outlives the object
// Build: make 11_dangling
#include <cstdint>
#include <iostream>
#include <memory>
#include <utility>

int main() {
    int* p = new int(7);
    int* q = p;                        // two pointers, one object
    std::cout << (p == q) << '\n';     // 1: the same address
    const auto old_address = reinterpret_cast<std::uintptr_t>(q);   // saved as a plain number, before the delete
    delete p;                          // the object ends, the number stays
    p = nullptr;                       // p can be tested now
    // q still holds the old address: a dangling pointer
    // *q = 1;                         // undefined behaviour: never run
    // the same bug: returning the address of a local variable
    auto owner = std::make_unique<int>(7);   // one owner, frees itself
    std::cout << *owner << '\n';

    // q is never read again: a dangling pointer is not even looked at, so we print the saved number
    std::cout << "\np == nullptr: " << (p == nullptr) << "\n";
    std::cout << "the number q still holds: 0x" << std::hex << old_address << std::dec
              << " (no object lives there now)\n";
    // int* make() { int local = 7; return &local; }   // invalid: the frame of make() is gone after the return
    std::unique_ptr<int> moved = std::move(owner);      // ownership moves, it is never copied
    std::cout << "moved holds " << *moved << ", sizeof(std::unique_ptr<int>) = " << sizeof(moved)
              << ": one address, no overhead\n";
    return 0;
}
