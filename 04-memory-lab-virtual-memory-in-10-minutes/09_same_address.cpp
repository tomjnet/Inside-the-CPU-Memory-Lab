// Virtual Memory in 10 Minutes: How Every Process Gets Its Own Memory - slide 9: same address, two processes
// Build: make 09_same_address
#include <cstdint>
#include <iostream>

#if defined(__linux__)
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#endif

// One line per process, flushed at once: the child leaves with _exit, which does not flush std::cout.
static void show(const char* who, const int* p, int v) {
    std::cout << "  " << who << "  &value = 0x" << std::hex << reinterpret_cast<std::uintptr_t>(p) << std::dec
              << "  value = " << v << std::endl;
}

int value = 1;                          // one global, one address

#if defined(__linux__)
void same_address() {
    pid_t child = fork();               // copies the map, not the RAM
    if (child == 0) {                   // child: a private space
        value = 42;                     // copy on write: 1 new page
        show("child ", &value, value);
        _exit(0);
    }
    waitpid(child, nullptr, 0);         // parent waits for the child
    show("parent", &value, value);      // same address, still 1
}
#endif

int main() {
    std::cout << "before fork:" << std::endl;
    show("parent", &value, value);

#if defined(__linux__)
    std::cout << "after fork, the child writes 42 into the global:" << std::endl;
    same_address();
    std::cout << "same virtual address, two values: two frames of RAM, one per process" << std::endl;
    std::cout << "(the page was shared until the write: copy on write)" << std::endl;
#else
    std::cout << "this sample needs Linux: fork() would print the same address twice, with 42 in the child\n";
    std::cout << "and 1 in the parent, because each process has its own address space\n";
#endif
    return 0;
}
