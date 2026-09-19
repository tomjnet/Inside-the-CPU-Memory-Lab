// Virtual Memory in 10 Minutes: How Every Process Gets Its Own Memory - slide 14: thank you
// Build: make 14_while_alive
#include <cstddef>
#include <iostream>
#include <string>
#include <vector>

// The imaginary "tomjnet.h" of the slide, written out so the program builds.
struct Topic { std::string name; };
struct Page { std::string title; bool resident = false; };

static std::size_t touched = 0;
static const std::size_t kEpisodes = 3;                      // three episodes, then the demo ends
static bool alive() { return touched < kEpisodes; }

struct AddressSpace {
    std::vector<Page> pages;                                 // reserved: titles only, nothing resident
    Page& touch() {                                          // first touch: the page becomes real
        Page& page = pages[touched++];
        page.resident = true;
        std::cout << "  page fault: \"" << page.title << "\" is now resident\n";
        return page;
    }
};

static Topic next_episode() { return Topic{"Pages and Page Tables: How Virtual Addresses Reach RAM"}; }

static AddressSpace reserve(const Topic& next) {
    AddressSpace space;
    space.pages = {Page{next.name}, Page{"CPU Cache Explained: L1, L2, L3 and Cache Lines"},
                   Page{"Memory Alignment and Locality: Why Data Layout Matters"}};
    std::cout << "reserved " << space.pages.size() << " episodes of the Memory Lab, none resident yet\n";
    return space;
}

static void learn(const Page& page) { std::cout << "  learned: " << page.title << '\n'; }
static void subscribe() { std::cout << "  subscribed to TomJNet\n"; }

int main() {
    AddressSpace mine = reserve(next_episode());  // no RAM yet
    while (alive()) {
        Page& page = mine.touch();       // page fault: now it is real
        learn(page);                     // 4 KiB at a time
        subscribe();                     // lifetime mapping
    }
    return 0;
}
