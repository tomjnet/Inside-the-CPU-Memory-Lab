// Memory Addresses and Pointers: What Is Really Stored? - slide 6: little endian: the bytes of an int
// Build: make 06_dump_bytes
#include <cstddef>
#include <cstdint>
#include <iostream>

int main() {
    int v = 0x11223344;
    auto* b = reinterpret_cast<const unsigned char*>(&v);
    for (std::size_t i = 0; i < sizeof v; ++i) {     // 4 reads, 1 byte each
        std::cout << static_cast<const void*>(b + i) << ": "
                  << std::hex << int(b[i]) << '\n';
    }
    // little endian: 44 33 22 11, lowest byte at the lowest address
    // unsigned char* may look at the bytes of any object

    // put the bytes back together by hand, low byte first: it must give the value we started with
    std::uint32_t rebuilt = 0;
    for (std::size_t i = 0; i < sizeof v; ++i) {
        rebuilt |= static_cast<std::uint32_t>(b[i]) << (8 * static_cast<unsigned>(i));
    }
    std::cout << "\nrebuilt low byte first: 0x" << rebuilt << "\n";
    const bool little = (b[0] == 0x44);
    std::cout << "this machine is " << (little ? "little" : "big") << " endian\n";

    // network byte order is big endian: the most significant byte goes first on the wire
    const std::uint32_t u = static_cast<std::uint32_t>(v);
    const unsigned char wire[4] = {static_cast<unsigned char>((u >> 24) & 0xffu), static_cast<unsigned char>((u >> 16) & 0xffu),
                                   static_cast<unsigned char>((u >> 8) & 0xffu), static_cast<unsigned char>(u & 0xffu)};
    std::cout << "on the wire (big endian):";
    for (unsigned char byte : wire) std::cout << ' ' << int(byte);
    std::cout << std::dec << "\n";
    if (little && rebuilt != u) return 1;
    return 0;
}
