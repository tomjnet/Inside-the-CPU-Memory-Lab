#!/usr/bin/env bash
# Virtual Memory in 10 Minutes - slide 8: reading the map.
# Look at the permissions column (r-xp is code, rw-p is data) and at the two address families: 55 and 7f.
set -eu

# the same list for any process: replace self with a PID
cat /proc/self/maps
# typical lines, trimmed (the addresses change on every run: ASLR)
#   55d0c4a00000-55d0c4a01000 r-xp  /usr/bin/cat     text
#   55d0c4c01000-55d0c4c02000 rw-p  /usr/bin/cat     data
#   55d0c5e3b000-55d0c5e5c000 rw-p  [heap]
#   7f3a1c000000-7f3a1c1d5000 r-xp  libc.so.6        mmap region
#   7ffc8a1e0000-7ffc8a201000 rw-p  [stack]
# one line per region with its size and its resident part, in KiB
pmap -x $$ | tail -n 3
