// Stack vs Heap: Where Does Your Data Actually Live? - slide 10: lifetime and ownership
// Build: make 10_lifetime
#include <iostream>

int* dangling() {
    int local = 42;          // dies at the closing brace
    std::cout << "  local = " << local << " at " << &local << ": valid only inside this call\n";   // added
    // return &local;        // frame popped: the pointer dangles
    // (kept as a comment: the compiler warns, and reading through it is undefined behaviour)
    return nullptr;
}
int* make_answer() {
    int* p = new int(42);    // lives until delete, not until the brace
    return p;                // the caller owns it now: who deletes?
}
// forget the delete: a leak.  delete twice: undefined behaviour.
// use after delete: undefined behaviour, often silent.

int main() {
    std::cout << "stack: the lifetime is the scope\n";
    int* gone = dangling();
    std::cout << "  dangling() returned " << (gone == nullptr ? "nullptr" : "a pointer")
              << ": the honest answer, the local no longer exists\n\n";

    std::cout << "heap: the lifetime is yours\n";
    int* answer = make_answer();
    std::cout << "  make_answer() returned " << answer << " holding " << *answer
              << ": still alive after the function returned\n";
    delete answer;           // exactly one delete, by the owner: main
    answer = nullptr;        // never read it again: use after delete is undefined
    std::cout << "  deleted once by its owner, pointer reset to "
              << (answer == nullptr ? "nullptr" : "?") << "\n\n";

    std::cout << "No delete: the block leaks until the process ends. Two deletes or a read after\n";
    std::cout << "the delete: undefined behaviour. Those lines are described, never executed.\n";
    return 0;
}
