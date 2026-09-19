# Inside the CPU: Memory Lab

Runnable code from the [Inside the CPU: Memory Lab](https://www.youtube.com/@TomJNet) video series: addresses and pointers, stack and heap, virtual memory, page tables, caches, alignment and a small allocator.
One folder per episode, one complete C++20 program per idea shown on the slides, and next to each one its C17 twin (`NN_name.c` beside `NN_name.cpp`) that teaches the same idea in plain C, plus the shell scripts that run the Linux tools of the episode.

| Folder | Video |
|---|---|
| `01-memory-lab-registers-to-ram-in-5-minutes` | Computer Memory in 5 Minutes: From Registers to RAM. The slide snippets as full programs. |
| `02-memory-lab-addresses-and-pointers-in-10-minutes` | Memory Addresses and Pointers: What Is Really Stored? The slide snippets as full programs. |
| `03-memory-lab-stack-vs-heap-in-10-minutes` | Stack vs Heap: Where Does Your Data Actually Live? The slide snippets as full programs. |
| `04-memory-lab-virtual-memory-in-10-minutes` | Virtual Memory in 10 Minutes: How Every Process Gets Its Own Memory. The slide snippets as full programs, plus the lab scripts. |
| `05-memory-lab-pages-and-page-tables-in-10-minutes` | Pages and Page Tables: How Virtual Addresses Reach RAM. The slide snippets as full programs. |
| `06-memory-lab-cpu-cache-l1-l2-l3-in-10-minutes` | CPU Cache Explained: L1, L2, L3 and Cache Lines. The slide snippets as full programs. |
| `07-memory-lab-alignment-and-locality-in-10-minutes` | Memory Alignment and Locality: Why Data Layout Matters. The slide snippets as full programs. |
| `08-memory-lab-simple-allocator-in-10-minutes` | Build a Simple Memory Allocator: Understanding malloc and new. The slide snippets as full programs. |
| `09-memory-lab-cache-misses-false-sharing-numa-in-10-minutes` | Memory Performance: Cache Misses, False Sharing and NUMA. The slide snippets as full programs, plus the lab scripts. |
| `10-memory-lab-series-summary-in-3-minutes` | Inside the CPU: What We Learned About Memory. The slide snippets as full programs. |

## The lab

- C++20 with `g++` on Ubuntu 24.04, x86 64 hardware. WSL Ubuntu is fine for writing and running most samples.
- Take timings on real Linux hardware: a virtual machine shares cores and hides the counters `perf` reads, and WSL shows no real PCIe devices, IRQs or NUMA nodes.
- Linux system calls sit inside `#if defined(__linux__)`. On Windows the programs still build and run, and print what the Linux branch would show.
- Every timing program prints its own numbers for the machine it runs on and never fails because of them. A call the system refuses (no root, one NUMA node, a filesystem without direct I/O) prints why and moves on.
- Hardware a program cannot touch is a small C++ model with deterministic output: read it, run it, change it.

## Build and run

Ubuntu or WSL Ubuntu:

```bash
sudo apt update
sudo apt install build-essential make linux-tools-common linux-tools-generic
```

Every folder has the same Makefile, for both languages. It picks the compilers by operating system: MSVC on Windows (run from a Developer Command Prompt), g++ and gcc on Linux, clang++ and clang on macOS.

```bash
cd 03-memory-lab-stack-vs-heap-in-10-minutes
make STD=c++20          # build everything
make run STD=c++20      # build and run every sample
make cpp STD=c++20      # only the C++ samples
make c                  # only the C samples (C17; make CSTD=c11 to change)
make run-NN_name_c      # one C sample: NN_name.c builds NN_name_c
make clean
```

The C samples build with `gcc -std=c17 -Wall -Wextra -pedantic -O2 -pthread` on Linux and MinGW, and with `cl /std:c17 /experimental:c11atomics /W4` on Windows. They use C11 `<threads.h>` and `<stdatomic.h>`; MinGW has no `<threads.h>`, so the files that start threads carry a ten line mapping onto pthreads.

The `.sh` scripts need Linux; some commands in them need `sudo` or real hardware and say so.

## License

MIT
