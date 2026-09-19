// CPU Cache Explained: L1, L2, L3 and Cache Lines - slide 8: read the cache sizes of your machine
// Build: make 08_cache_sizes
#include <iostream>
#include <string>

#if defined(__linux__)
#include <fstream>

std::string read_line(const std::string& path) {
    std::ifstream f(path);              // one small text file
    std::string s; std::getline(f, s);  // per fact, no parsing
    return s;                           // empty: file is missing
}
#endif

static void print_typical() {
    std::cout << "typical values of a desktop x86 64 CPU (orders of magnitude, not this machine):\n";
    std::cout << "  L1 Data         32K to 48K    line 64   per core     about 1 ns\n";
    std::cout << "  L1 Instruction  32K to 48K    line 64   per core     about 1 ns\n";
    std::cout << "  L2 Unified      256K to 2048K line 64   per core     about 4 ns\n";
    std::cout << "  L3 Unified      8M to 64M     line 64   shared       10 to 40 ns\n";
}

int main() {
#if defined(__linux__)
    const std::string base = "/sys/devices/system/cpu/cpu0/cache";
    std::cout << base << " (this machine):\n";

    for (int i = 0; i < 4; ++i) {           // L1 data, L1 code, L2, L3
        std::string d = base + "/index" + std::to_string(i) + "/";
        std::cout << "L" << read_line(d + "level") << " "
                  << read_line(d + "type") << " " << read_line(d + "size")
                  << " line " << read_line(d + "coherency_line_size")
                  << "\n";                  // typical: L1 Data 48K line 64
    }

    // who shares each cache: L1 and L2 list the hyperthreads of one core, L3 lists every core of the chip
    int found = 0;
    for (int i = 0; i < 4; ++i) {
        const std::string d = base + "/index" + std::to_string(i) + "/";
        const std::string level = read_line(d + "level");
        if (level.empty()) continue;
        ++found;
        std::cout << "  index" << i << ": L" << level << " shared by CPUs " << read_line(d + "shared_cpu_list")
                  << ", " << read_line(d + "ways_of_associativity") << " ways, "
                  << read_line(d + "number_of_sets") << " sets\n";
    }
    if (found == 0) {
        std::cout << "no cache folders here: a virtual machine or WSL can hide them (the lines above are empty).\n";
        std::cout << "on real Linux hardware you would see four lines such as: L1 Data 48K line 64\n";
        print_typical();
    } else if (found < 4) {
        std::cout << "only " << found << " of 4 cache folders are visible here: virtual machines often hide a level\n";
    }
    std::cout << "the same table: lscpu --caches\n";
#else
    std::cout << "this sample needs Linux: it would print level, type, size and coherency_line_size of the four\n";
    std::cout << "caches of cpu0 from /sys/devices/system/cpu/cpu0/cache/index0 to index3\n";
    print_typical();
#endif
    return 0;
}
