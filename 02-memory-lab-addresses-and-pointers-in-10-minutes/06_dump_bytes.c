/* Memory Addresses and Pointers: What Is Really Stored? - slide 6: little endian: the bytes of an int (C version of 06_dump_bytes.cpp) */
/* Build: make 06_dump_bytes_c */
#if defined(__linux__)
#define _GNU_SOURCE
#endif
#if defined(_MSC_VER)
#define _CRT_SECURE_NO_WARNINGS
#endif

#include <inttypes.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

int main(void) {
    int v = 0x11223344;
    const unsigned char *b = (const unsigned char *)&v;
    for (size_t i = 0; i < sizeof v; ++i) {          /* 4 reads, 1 byte each */
        printf("%p: %x\n", (const void *)(b + i), (unsigned)b[i]);
    }
    /* little endian: 44 33 22 11, lowest byte at the lowest address */
    /* unsigned char * may look at the bytes of any object */

    /* put the bytes back together by hand, low byte first: it must give the value we started with */
    uint32_t rebuilt = 0;
    for (size_t i = 0; i < sizeof v; ++i) {
        rebuilt |= (uint32_t)b[i] << (8u * (unsigned)i);
    }
    printf("\nrebuilt low byte first: 0x%" PRIx32 "\n", rebuilt);
    const int little = (b[0] == 0x44);
    printf("this machine is %s endian\n", little ? "little" : "big");

    /* network byte order is big endian: the most significant byte goes first on the wire */
    const uint32_t u = (uint32_t)v;
    const unsigned char wire[4] = {(unsigned char)((u >> 24) & 0xffu), (unsigned char)((u >> 16) & 0xffu),
                                   (unsigned char)((u >> 8) & 0xffu), (unsigned char)(u & 0xffu)};
    printf("on the wire (big endian):");
    for (size_t i = 0; i < sizeof wire; ++i) printf(" %x", (unsigned)wire[i]);
    printf("\n");
    if (little && rebuilt != u) return 1;
    return 0;
}
