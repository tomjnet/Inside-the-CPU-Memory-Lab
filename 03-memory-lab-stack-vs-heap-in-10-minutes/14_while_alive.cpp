// Stack vs Heap: Where Does Your Data Actually Live? - slide 14: thank you
// Build: make 14_while_alive
#include <iostream>
#include <memory>
#include <string>
#include <vector>

// the imaginary tomjnet.h
struct Topic { std::string name; };
static int episodes = 0;
static bool alive() { return episodes < 3; }                 // three episodes, then the demo ends
static Topic next_memory_topic() {
    static const char* topics[] = {"Virtual Memory in 10 Minutes", "Pages and Page Tables",
                                   "CPU Cache Explained: L1, L2, L3 and Cache Lines"};
    return Topic{topics[episodes++]};
}
static Topic viewer_request() { return Topic{"viewer request #" + std::to_string(episodes)}; }
static void subscribe() { std::cout << "  subscribed to TomJNet\n"; }

int main() {
    auto lab = std::make_unique<std::vector<Topic>>();  // one owner
    while (alive()) {
        lab->push_back(next_memory_topic());  // next: virtual memory
        lab->push_back(viewer_request());     // on the heap, owned
        subscribe();                          // lifetime benefit
    }
    std::cout << "the Memory Lab queue, owned by one unique_ptr on the stack:\n";
    for (const Topic& t : *lab) std::cout << "  " << t.name << "\n";
    return 0;
}                                             // delete runs here
