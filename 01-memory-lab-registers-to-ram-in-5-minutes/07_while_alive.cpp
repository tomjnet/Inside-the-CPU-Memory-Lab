// Computer Memory in 5 Minutes: From Registers to RAM - slide 7: thank you
// Build: make 07_while_alive
#include <iostream>
#include <string>
#include <vector>

// #include "tomjnet.h"   (the imaginary header of the slide: this block stands in for it)
struct Topic { std::string name; };
static int episodes = 0;
static bool alive() { return episodes < 3; }                 // three episodes, then the demo ends
static Topic next_memory_topic() {
    static const char* topics[] = {"Memory Addresses and Pointers: What Is Really Stored?",
                                   "Stack vs Heap: Where Does Your Data Actually Live?",
                                   "Virtual Memory in 10 Minutes"};
    return Topic{topics[episodes++]};
}
static Topic viewer_request() { return Topic{"viewer request #" + std::to_string(episodes)}; }
static void subscribe() { std::cout << "  subscribed to TomJNet\n"; }

int main() {
    std::vector<Topic> memory_lab;
    while (alive()) {
        memory_lab.push_back(next_memory_topic()); // contiguous
        memory_lab.push_back(viewer_request());    // next cache line
        subscribe();                               // lifetime benefit
    }

    std::cout << "the Memory Lab queue, " << memory_lab.size() << " topics in one contiguous block:\n";
    for (const Topic& t : memory_lab) std::cout << "  " << t.name << "\n";
    return 0;
}
