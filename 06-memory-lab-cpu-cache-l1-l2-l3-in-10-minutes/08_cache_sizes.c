/* CPU Cache Explained: L1, L2, L3 and Cache Lines - slide 8: read the cache sizes of your machine (C version of 08_cache_sizes.cpp) */
/* Build: make 08_cache_sizes_c */
#if defined(__linux__)
#define _GNU_SOURCE                  /* sched_setaffinity, CPU_SET, O_DIRECT, clock_gettime */
#endif
#if defined(_MSC_VER)
#define _CRT_SECURE_NO_WARNINGS      /* fopen, strerror: MSVC deprecates the standard names */
#endif

#include <stdio.h>
#include <string.h>

#if defined(__linux__)
/* one small text file per fact, no parsing; out stays empty when the file is missing */
static const char *read_line(const char *dir, const char *name, char *out, size_t cap) {
    char path[256];
    snprintf(path, sizeof path, "%s%s", dir, name);
    out[0] = '\0';
    FILE *f = fopen(path, "r");
    if (f == NULL) return out;
    if (fgets(out, (int)cap, f) == NULL) out[0] = '\0';
    fclose(f);
    out[strcspn(out, "\n")] = '\0';     /* fgets keeps the newline, std::getline does not */
    return out;
}
#endif

static void print_typical(void) {
    printf("typical values of a desktop x86 64 CPU (orders of magnitude, not this machine):\n");
    printf("  L1 Data         32K to 48K    line 64   per core     about 1 ns\n");
    printf("  L1 Instruction  32K to 48K    line 64   per core     about 1 ns\n");
    printf("  L2 Unified      256K to 2048K line 64   per core     about 4 ns\n");
    printf("  L3 Unified      8M to 64M     line 64   shared       10 to 40 ns\n");
}

int main(void) {
#if defined(__linux__)
    const char *base = "/sys/devices/system/cpu/cpu0/cache";
    printf("%s (this machine):\n", base);

    char d[128], a[128], b[128], c[128], e[128];
    for (int i = 0; i < 4; ++i) {           /* L1 data, L1 code, L2, L3 */
        snprintf(d, sizeof d, "%s/index%d/", base, i);
        printf("L%s %s %s line %s\n", read_line(d, "level", a, sizeof a), read_line(d, "type", b, sizeof b),
               read_line(d, "size", c, sizeof c),
               read_line(d, "coherency_line_size", e, sizeof e));   /* typical: L1 Data 48K line 64 */
    }

    /* who shares each cache: L1 and L2 list the hyperthreads of one core, L3 lists every core of the chip */
    int found = 0;
    for (int i = 0; i < 4; ++i) {
        snprintf(d, sizeof d, "%s/index%d/", base, i);
        read_line(d, "level", a, sizeof a);
        if (a[0] == '\0') continue;
        ++found;
        printf("  index%d: L%s shared by CPUs %s, %s ways, %s sets\n", i, a,
               read_line(d, "shared_cpu_list", b, sizeof b), read_line(d, "ways_of_associativity", c, sizeof c),
               read_line(d, "number_of_sets", e, sizeof e));
    }
    if (found == 0) {
        printf("no cache folders here: a virtual machine or WSL can hide them (the lines above are empty).\n");
        printf("on real Linux hardware you would see four lines such as: L1 Data 48K line 64\n");
        print_typical();
    } else if (found < 4) {
        printf("only %d of 4 cache folders are visible here: virtual machines often hide a level\n", found);
    }
    printf("the same table: lscpu --caches\n");
#else
    printf("this sample needs Linux: it would print level, type, size and coherency_line_size of the four\n");
    printf("caches of cpu0 from /sys/devices/system/cpu/cpu0/cache/index0 to index3\n");
    print_typical();
#endif
    return 0;
}
