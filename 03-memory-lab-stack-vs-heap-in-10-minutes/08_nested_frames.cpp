// Stack vs Heap: Where Does Your Data Actually Live? - slide 8: lab: locals in nested calls descend
// Build: make 08_nested_frames
#include <cstdint>
#include <iostream>

int frame(int depth, std::uintptr_t caller);
int (*volatile call)(int, std::uintptr_t) = frame;   // never inlined

int frame(int depth, std::uintptr_t caller) {
    int local = depth;                       // lives in this frame
    auto here = reinterpret_cast<std::uintptr_t>(&local);
    std::cout << "depth " << depth << "  &local " << &local
              << "  below the caller by " << caller - here << " B\n";
    int deeper = depth < 4 ? call(depth + 1, here) : 0;  // new frame
    return local + deeper;                   // frame popped: rsp back
}

int main() {
    int in_main = 0;
    auto top = reinterpret_cast<std::uintptr_t>(&in_main);
    std::cout << "main     &local " << &in_main << "\n";

    int sum = call(1, top);                  // 1 + 2 + 3 + 4: every frame returned its local

    std::cout << "sum of the locals on the way back: " << sum + in_main << "\n\n";
    std::cout << "Every address is lower than the one before: the stack grows down.\n";
    std::cout << "The distance between two depths is one frame: return address, saved registers,\n";
    std::cout << "the local and alignment padding. The first gap also holds what main keeps in its frame.\n";
    std::cout << "The numbers change with the compiler and the flags; the direction does not.\n";
    return 0;
}
