// Virtual Memory in 10 Minutes: How Every Process Gets Its Own Memory - slide 7: /proc/self/maps: the kernel's own list
// Build: make 07_proc_maps
#include <cstdint>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

// One line of /proc/<pid>/maps: "begin-end perms offset dev inode [path]". Portable: it only parses text.
struct Region {
    std::uint64_t begin = 0;
    std::uint64_t end = 0;
    std::string perms;
    std::string path;
};

static bool parse(const std::string& line, Region& r) {
    std::istringstream in(line);
    std::string range, offset, dev, inode;
    if (!(in >> range >> r.perms >> offset >> dev >> inode)) return false;
    const std::size_t dash = range.find('-');
    if (dash == std::string::npos) return false;
    r.begin = std::stoull(range.substr(0, dash), nullptr, 16);
    r.end = std::stoull(range.substr(dash + 1), nullptr, 16);
    r.path.clear();
    in >> r.path;                           // empty for an anonymous region
    return r.end > r.begin;
}

static void describe(const Region& r) {
    const char* kind = r.perms.find('x') != std::string::npos ? "code"
                     : r.perms.find('w') != std::string::npos ? "read and write data"
                                                              : "read only data";
    std::cout << "  " << r.perms << "  " << (r.end - r.begin) / 1024 << " KiB  " << kind << "  "
              << (r.path.empty() ? "(anonymous)" : r.path) << '\n';
}

#if defined(__linux__)
static bool interesting(const std::string& line) {
    return line.find("07_proc_maps") != std::string::npos || line.find("[heap]") != std::string::npos ||
           line.find("libc") != std::string::npos || line.find("[stack]") != std::string::npos;
}

// /proc/self/maps: the kernel's list of the regions of this process
// one line per region: address range, permissions, offset, file
void print_maps() {
    std::ifstream maps("/proc/self/maps");  // text built on demand
    std::string line;
    while (std::getline(maps, line)) {      // a few read() calls,
        if (interesting(line))              // one per buffer, not
            std::cout << line << '\n';      // one per line
    }
}
#endif

int main() {
#if defined(__linux__)
    std::cout << "the lines of /proc/self/maps about this program, [heap], libc and [stack]:\n";
    print_maps();

    std::ifstream maps("/proc/self/maps");
    if (!maps) {
        std::cout << "/proc/self/maps is not readable here: on a normal Linux it lists every region\n";
        return 0;
    }
    std::string line;
    Region r;
    std::uint64_t regions = 0, mapped = 0;
    std::cout << "\nthe same regions, decoded:\n";
    while (std::getline(maps, line)) {
        if (!parse(line, r)) continue;
        ++regions;
        mapped += r.end - r.begin;
        if (interesting(line)) describe(r);
    }
    std::cout << "\n" << regions << " regions, " << mapped / 1024 << " KiB of address space mapped (virtual, not RAM)\n";
#else
    std::cout << "this sample needs Linux: it would print the regions of its own process from /proc/self/maps\n";
    std::cout << "typical lines (the addresses change on every run), decoded by the same parser:\n";
    const char* typical[] = {
        "55d0c4a00000-55d0c4a01000 r-xp 00001000 08:02 1311 /usr/bin/cat",
        "55d0c4c01000-55d0c4c02000 rw-p 00003000 08:02 1311 /usr/bin/cat",
        "55d0c5e3b000-55d0c5e5c000 rw-p 00000000 00:00 0 [heap]",
        "7f3a1c000000-7f3a1c1d5000 r-xp 00028000 08:02 2718 /usr/lib/x86_64-linux-gnu/libc.so.6",
        "7ffc8a1e0000-7ffc8a201000 rw-p 00000000 00:00 0 [stack]",
    };
    Region r;
    for (const char* line : typical) {
        std::cout << line << '\n';
        if (parse(line, r)) describe(r);
    }
#endif
    return 0;
}
