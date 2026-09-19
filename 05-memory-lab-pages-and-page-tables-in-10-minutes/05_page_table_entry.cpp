// Pages and Page Tables: How Virtual Addresses Reach RAM - slide 5: inside a page table entry
// Build: make 05_page_table_entry
#include <cstdint>
#include <iostream>

// model of a page table entry: 8 bytes that map one 4 KiB page
struct Entry {
    std::uint64_t present  : 1;   // 0: the access is a page fault
    std::uint64_t writable : 1;   // 0: a write faults (copy on write)
    std::uint64_t user     : 1;   // 0: kernel only
    std::uint64_t accessed : 1;   // set by the CPU on any access
    std::uint64_t dirty    : 1;   // set by the CPU on a write
    std::uint64_t frame    : 40;  // physical frame number
    std::uint64_t no_exec  : 1;   // 1: fetching code here faults
};
// 512 entries x 8 bytes = 4096 bytes: a table is itself one page

// A model: the real x86 64 entry keeps present, writable and user in bits 0 to 2, accessed and dirty in bits
// 5 and 6, the frame number from bit 12 up and no execute in bit 63. The fields are the same.
static_assert(sizeof(Entry) == 8, "one entry is 8 bytes");
static_assert(sizeof(Entry) * 512 == 4096, "a table of 512 entries is one page");

enum class Access { Read, Write, Execute };

static const char* name(Access a) {
    return a == Access::Read ? "read   " : a == Access::Write ? "write  " : "execute";
}

// What the CPU does with one entry on one access: check the flags, then set accessed and dirty.
static bool access(Entry& e, Access a, bool user_mode) {
    const char* why = nullptr;
    if (!e.present) why = "not present";
    else if (user_mode && !e.user) why = "kernel only page";
    else if (a == Access::Write && !e.writable) why = "read only page";
    else if (a == Access::Execute && e.no_exec) why = "no execute page";
    std::cout << "  " << name(a) << (why ? " -> page fault: " : " -> ok") << (why ? why : "") << "\n";
    if (why) return false;
    e.accessed = 1;
    if (a == Access::Write) e.dirty = 1;
    return true;
}

static void show(const char* label, const Entry& e) {
    std::cout << label << ": present=" << e.present << " writable=" << e.writable << " user=" << e.user
              << " accessed=" << e.accessed << " dirty=" << e.dirty << " no_exec=" << e.no_exec << " frame=0x"
              << std::hex << e.frame << std::dec << "\n";
}

int main() {
    std::cout << "sizeof(Entry) = " << sizeof(Entry) << " bytes, 512 entries = " << sizeof(Entry) * 512
              << " bytes: a table is itself one page\n\n";

    Entry data{};                       // a page of a writable variable
    data.present = 1; data.writable = 1; data.user = 1; data.no_exec = 1; data.frame = 0x1a2b3;
    show("data page  ", data);
    int faults = 0;
    if (!access(data, Access::Read, true)) ++faults;
    if (!access(data, Access::Write, true)) ++faults;
    if (!access(data, Access::Execute, true)) ++faults;       // the only fault of the three
    show("after      ", data);

    Entry text{};                       // a page of code: read and execute, never write
    text.present = 1; text.user = 1; text.frame = 0x0c411;
    std::cout << "\n";
    show("code page  ", text);
    if (!access(text, Access::Execute, true)) ++faults;
    if (!access(text, Access::Write, true)) ++faults;         // fault: read only

    Entry fresh{};                      // mapped by mmap, never touched: the first access is a minor fault
    std::cout << "\n";
    show("fresh page ", fresh);
    if (!access(fresh, Access::Read, true)) ++faults;         // fault: not present

    std::cout << "\n" << faults << " page faults out of 6 accesses\n"
              << "protection is per page: the flags live in the entry, and an entry maps 4096 bytes at once\n";
    return (faults == 3 && data.accessed && data.dirty && !text.dirty) ? 0 : 1;
}
