// Memory Performance: Cache Misses, False Sharing and NUMA - slide 13: thank you
// Build: make 13_while_alive
#include <iostream>
#include <string>
#include <vector>

// the imaginary tomjnet.h of the slide
struct Topic { std::string name; };
static int episodes = 0;
static bool alive() { return episodes < 3; }                 // three episodes, then the demo ends
static Topic next_memory_topic() {
    static const char* topics[] = {"Inside the CPU: What We Learned About Memory (episode 10)",
                                   "the Low Latency C++ Lab", "the Cache and NUMA Lab"};
    return Topic{topics[episodes++]};
}
static Topic viewer_request() { return Topic{"viewer request #" + std::to_string(episodes)}; }
static void subscribe() { std::cout << "  subscribed to TomJNet\n"; }

int main() {
    std::vector<Topic> memory_lab;
    while (alive()) {
        memory_lab.push_back(next_memory_topic()); // contiguous
        memory_lab.push_back(viewer_request());    // same cache line
        subscribe();                               // lifetime benefit
    }

    std::cout << "next in the Memory Lab and after it:\n";
    for (const Topic& t : memory_lab) std::cout << "  " << t.name << "\n";
    return 0;
}
