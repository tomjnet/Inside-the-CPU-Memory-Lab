// Memory Addresses and Pointers: What Is Really Stored? - slide 13: thank you
// Build: make 13_while_alive
#include <iostream>
#include <string>
#include <vector>

// #include "tomjnet.h"   (the imaginary header of the slide: this block stands in for it)
struct Topic { std::string name; };
static int episodes = 0;
static bool alive() { return episodes < 3; }                 // three episodes, then the demo ends
static Topic next_memory_topic() {
    static const char* topics[] = {"Stack vs Heap: Where Does Your Data Actually Live?",
                                   "Virtual Memory in 10 Minutes: How Every Process Gets Its Own Memory",
                                   "Pages and Page Tables: How Virtual Addresses Reach RAM"};
    return Topic{topics[episodes++]};
}
static void watch(const Topic* t) {
    std::cout << "  watching " << static_cast<const void*>(t) << ": " << t->name << "\n";
}
static void subscribe() { std::cout << "  subscribed to TomJNet\n"; }

int main() {
    std::vector<Topic> memory_lab;
    while (alive()) {
        memory_lab.push_back(next_memory_topic()); // stack vs heap
        Topic* latest = &memory_lab.back();        // address + type
        watch(latest);                             // never nullptr
        subscribe();                               // lifetime benefit
    }

    std::cout << "the Memory Lab queue, " << memory_lab.size() << " topics:\n";
    for (const Topic& t : memory_lab) std::cout << "  " << t.name << "\n";
    return 0;
}
