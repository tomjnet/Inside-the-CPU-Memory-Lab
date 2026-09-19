#!/usr/bin/env bash
# Look at: cache-misses as a share of cache-references for the false sharing run, then page-faults close to
# one per 4 KiB page for the page touch run. Needs real Linux hardware: a VM and WSL hide these counters.
set -eu

# build as every timing in this lab: optimization level two
g++ -std=c++20 -O2 -pthread 08_false_sharing.cpp -o false_sharing
# (the slide shows one build line; the other two programs of this script are built the same way)
g++ -std=c++20 -O2 04_page_faults.cpp -o page_faults
g++ -std=c++20 -O2 10_memcpy_bandwidth.cpp -o memcpy_bandwidth
# cache misses of the whole run (real Linux hardware, not a VM)
perf stat -e cache-references,cache-misses ./false_sharing
#   typical shape: cache-misses is a large share of cache-references
# TLB misses and page faults of the page touch sample
perf stat -e dTLB-load-misses,page-faults ./page_faults
#   typical shape: about 16384 page-faults per fresh 64 MiB buffer
# NUMA: list the nodes, then keep CPU and memory on node 0
numactl --hardware
numactl --cpunodebind=0 --membind=0 ./memcpy_bandwidth

# leave only sources behind
rm -f false_sharing page_faults memcpy_bandwidth
