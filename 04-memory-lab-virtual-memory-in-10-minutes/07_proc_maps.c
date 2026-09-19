/* Virtual Memory in 10 Minutes: How Every Process Gets Its Own Memory - slide 7: /proc/self/maps: the kernel's own list (C version of 07_proc_maps.cpp) */
/* Build: make 07_proc_maps_c */
#if defined(__linux__)
#define _GNU_SOURCE
#endif
#if defined(_MSC_VER)
#define _CRT_SECURE_NO_WARNINGS
#endif

#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* One line of /proc/<pid>/maps: "begin-end perms offset dev inode [path]". Portable: it only parses text. */
typedef struct {
    uint64_t begin;
    uint64_t end;
    char perms[8];
    char path[512];
} Region;

static int parse(const char *line, Region *r) {
    char range[64], offset[32], dev[32], inode[32];
    int used = 0;
    if (sscanf(line, "%63s %7s %31s %31s %31s%n", range, r->perms, offset, dev, inode, &used) != 5) return 0;
    char *dash = strchr(range, '-');
    if (dash == NULL) return 0;
    *dash = '\0';
    r->begin = strtoull(range, NULL, 16);
    r->end = strtoull(dash + 1, NULL, 16);
    r->path[0] = '\0';
    if (sscanf(line + used, "%511s", r->path) != 1)   /* empty for an anonymous region */
        r->path[0] = '\0';
    return r->end > r->begin;
}

static void describe(const Region *r) {
    const char *kind = strchr(r->perms, 'x') != NULL ? "code"
                     : strchr(r->perms, 'w') != NULL ? "read and write data"
                                                     : "read only data";
    printf("  %s  %" PRIu64 " KiB  %s  %s\n", r->perms, (r->end - r->begin) / 1024, kind,
           r->path[0] == '\0' ? "(anonymous)" : r->path);
}

#if defined(__linux__)
static int interesting(const char *line) {
    return strstr(line, "07_proc_maps") != NULL || strstr(line, "[heap]") != NULL ||
           strstr(line, "libc") != NULL || strstr(line, "[stack]") != NULL;
}

/* /proc/self/maps: the kernel's list of the regions of this process */
/* one line per region: address range, permissions, offset, file */
static void print_maps(void) {
    FILE *maps = fopen("/proc/self/maps", "r");   /* text built on demand */
    if (maps == NULL) return;
    char line[4096];
    while (fgets(line, sizeof line, maps) != NULL) {   /* a few read() calls, */
        if (interesting(line))                          /* one per buffer, not */
            fputs(line, stdout);                        /* one per line */
    }
    fclose(maps);
}
#endif

int main(void) {
#if defined(__linux__)
    printf("the lines of /proc/self/maps about this program, [heap], libc and [stack]:\n");
    print_maps();

    FILE *maps = fopen("/proc/self/maps", "r");
    if (maps == NULL) {
        printf("/proc/self/maps is not readable here: on a normal Linux it lists every region\n");
        return 0;
    }
    char line[4096];
    Region r;
    uint64_t regions = 0, mapped = 0;
    printf("\nthe same regions, decoded:\n");
    while (fgets(line, sizeof line, maps) != NULL) {
        if (!parse(line, &r)) continue;
        ++regions;
        mapped += r.end - r.begin;
        if (interesting(line)) describe(&r);
    }
    fclose(maps);
    printf("\n%" PRIu64 " regions, %" PRIu64 " KiB of address space mapped (virtual, not RAM)\n",
           regions, mapped / 1024);
#else
    printf("this sample needs Linux: it would print the regions of its own process from /proc/self/maps\n");
    printf("typical lines (the addresses change on every run), decoded by the same parser:\n");
    const char *typical[] = {
        "55d0c4a00000-55d0c4a01000 r-xp 00001000 08:02 1311 /usr/bin/cat",
        "55d0c4c01000-55d0c4c02000 rw-p 00003000 08:02 1311 /usr/bin/cat",
        "55d0c5e3b000-55d0c5e5c000 rw-p 00000000 00:00 0 [heap]",
        "7f3a1c000000-7f3a1c1d5000 r-xp 00028000 08:02 2718 /usr/lib/x86_64-linux-gnu/libc.so.6",
        "7ffc8a1e0000-7ffc8a201000 rw-p 00000000 00:00 0 [stack]",
    };
    Region r;
    for (size_t i = 0; i < sizeof typical / sizeof typical[0]; ++i) {
        printf("%s\n", typical[i]);
        if (parse(typical[i], &r)) describe(&r);
    }
#endif
    return 0;
}
