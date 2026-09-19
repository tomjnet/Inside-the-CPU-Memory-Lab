// Build a Simple Memory Allocator: Understanding malloc and new - slide 14: thank you
// Build: make 14_while_alive
#include <iostream>
#include <string>
#include <vector>

// #include "tomjnet.h"   (the imaginary header of the slide: this block stands in for it)
struct Topic { std::string name; };
static int episodes = 0;
static bool alive() { return episodes < 3; }                 // three episodes, then the demo ends
static Topic next_memory_topic() {
    static const char* topics[] = {"Memory Performance: Cache Misses, False Sharing and NUMA",
                                   "Inside the CPU: What We Learned About Memory",
                                   "Inside the CPU: Cache and NUMA Lab"};
    return Topic{topics[episodes++]};
}
static Topic viewer_request() { return Topic{"viewer request #" + std::to_string(episodes)}; }
static void subscribe() { std::cout << "  subscribed to TomJNet\n"; }

int main() {
    std::vector<Topic> memory_lab;
    while (alive()) {
        memory_lab.push_back(next_memory_topic()); // first fit
        memory_lab.push_back(viewer_request());    // split the rest
        subscribe();                               // never deallocate
    }

    std::cout << "the Memory Lab queue, " << memory_lab.size() << " topics:\n";
    for (const Topic& t : memory_lab) std::cout << "  " << t.name << "\n";
    return 0;
}
