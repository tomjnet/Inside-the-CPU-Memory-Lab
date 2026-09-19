// Pages and Page Tables: How Virtual Addresses Reach RAM - slide 13: thank you
// Build: make 13_while_alive
#include <iostream>
#include <string>
#include <vector>

// #include "tomjnet.h"   (the imaginary header of the slide: this block stands in for it)
struct Topic { std::string name; };
static int episodes = 0;
static bool alive() { return episodes < 3; }                 // three episodes, then the demo ends
static Topic next_memory_topic() {
    static const char* topics[] = {"CPU Cache Explained: L1, L2, L3 and Cache Lines",
                                   "Memory Alignment and Locality: Why Data Layout Matters",
                                   "Build a Simple Memory Allocator: Understanding malloc and new"};
    return Topic{topics[episodes++]};
}
static Topic viewer_request() { return Topic{"viewer request #" + std::to_string(episodes)}; }
static void subscribe() { std::cout << "  subscribed to TomJNet\n"; }

int main() {
    std::vector<Topic> memory_lab;
    while (alive()) {
        memory_lab.push_back(next_memory_topic()); // CPU cache next
        memory_lab.push_back(viewer_request());    // same page
        subscribe();                               // lifetime benefit
    }

    std::cout << "the Memory Lab queue, " << memory_lab.size() << " topics:\n";
    for (const Topic& t : memory_lab) std::cout << "  " << t.name << "\n";
    return 0;
}
