/* Pages and Page Tables: How Virtual Addresses Reach RAM - slide 2: the page: the unit of every mapping (C version of 02_page_and_offset.cpp) */
/* Build: make 02_page_and_offset_c */
#if defined(__linux__)
#define _GNU_SOURCE
#endif
#if defined(_MSC_VER)
#define _CRT_SECURE_NO_WARNINGS
#endif

#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>

int main(void) {
    /* a page is 4 KiB: 2 to the 12 bytes, the unit of every mapping */
    const uint64_t kPage = 4096;
    uint64_t addr   = 0x7ffd1234abcdULL;
    uint64_t page   = addr >> 12;          /* virtual page number */
    uint64_t offset = addr & (kPage - 1);  /* 0xbcd: never translated */
    /* one entry per page, not per byte: 1 GiB needs 262144 entries */
    uint64_t entries = (1ULL << 30) / kPage;
    /* per byte it would be 2 to the 30 entries of 8 bytes each:
       8 GiB of tables to map 1 GiB of memory */

    printf("address              0x%" PRIx64 "\n", addr);
    printf("virtual page number  0x%" PRIx64 "   (address >> 12: this part is translated)\n", page);
    printf("offset in the page   0x%" PRIx64 "         (address & 0xfff: copied as it is)\n", offset);
    printf("page start           0x%" PRIx64 "\n", page << 12);

    /* every byte of one page shares the page number: one entry serves all 4096 of them */
    uint64_t first = page << 12;
    uint64_t last = first + kPage - 1;
    printf("\nfirst and last byte of that page: page number 0x%" PRIx64 " and 0x%" PRIx64 "\n",
           first >> 12, last >> 12);
    if ((first >> 12) != (last >> 12)) {
        printf("error: one page, two page numbers\n");
        return 1;
    }

    const uint64_t gib = 1ULL << 30;
    uint64_t per_page_bytes = entries * 8;          /* last level entries only */
    uint64_t per_byte_bytes = gib * 8;              /* the loose claim, taken literally */
    printf("\nmapping 1 GiB\n");
    printf("  one entry per page: %" PRIu64 " entries, %" PRIu64 " MiB of tables\n",
           entries, per_page_bytes / (1u << 20));
    printf("  one entry per byte: %" PRIu64 " entries, %" PRIu64 " GiB of tables\n",
           gib, per_byte_bytes / gib);
    printf("a page table entry maps a page, not a byte\n");
    return 0;
}
