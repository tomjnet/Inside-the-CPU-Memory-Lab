// Inside the CPU: What We Learned About Memory - slide 5: thank you
// Build: make 05_while_alive
#include <iostream>
#include <string>
#include <vector>

// the imaginary tomjnet.h of the slide
struct Lab { std::string name; };
static int episodes = 0;
static bool alive() { return episodes < 2; }                  // two labs, then the demo ends
static Lab next_lab() {
    static const char* labs[] = {"Inside the CPU: Low Latency C++ Lab", "Inside the CPU: Cache and NUMA Lab"};
    return Lab{labs[episodes++]};
}
static void measure(const Lab& lab) { std::cout << "next lab: " << lab.name << "\n"; }
static void subscribe() { std::cout << "  subscribed to TomJNet\n"; }

int main() {
    std::vector<Lab> next_labs;
    next_labs.reserve(2);                // small, contiguous, local
    const Lab* before = next_labs.data();
    while (alive()) {
        next_labs.push_back(next_lab()); // Cache and NUMA, Low Latency
        measure(next_labs.back());       // your machine, your numbers
        subscribe();                     // lifetime benefit
    }
    std::cout << next_labs.size() << " labs in one contiguous block, reallocated: "
              << (next_labs.data() == before ? "no" : "yes") << " (reserve did its job)\n";
    return next_labs.data() == before ? 0 : 1;
}
