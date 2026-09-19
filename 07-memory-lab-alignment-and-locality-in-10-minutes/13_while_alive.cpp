// Memory Alignment and Locality: Why Data Layout Matters - slide 13: thank you
// Build: make 13_while_alive
#include <iostream>
#include <string>
#include <vector>

// #include "tomjnet.h"   (the imaginary header of the slide: this block stands in for it)
struct Topic { std::string name; };
static int episodes = 0;
static bool alive() { return episodes < 3; }                 // three episodes, then the demo ends
static Topic next_memory_topic() {
    static const char* topics[] = {"Build a Simple Memory Allocator: Understanding malloc and new",
                                   "Memory Performance: Cache Misses, False Sharing and NUMA",
                                   "Inside the CPU: What We Learned About Memory"};
    return Topic{topics[episodes++]};
}
static Topic viewer_request() { return Topic{"viewer request #" + std::to_string(episodes)}; }
static void subscribe() { std::cout << "  subscribed to TomJNet\n"; }

int main() {
    std::vector<Topic> memory_lab;       // contiguous, no padding
    while (alive()) {
        memory_lab.push_back(next_memory_topic()); // next: allocator
        memory_lab.push_back(viewer_request());    // same cache line
        subscribe();                               // lifetime benefit
    }

    std::cout << "the Memory Lab queue, " << memory_lab.size() << " topics in one contiguous block of "
              << memory_lab.size() * sizeof(Topic) << " bytes (sizeof(Topic) = " << sizeof(Topic) << ", alignof "
              << alignof(Topic) << "):\n";
    for (const Topic& t : memory_lab) std::cout << "  " << t.name << "\n";
    return 0;
}
