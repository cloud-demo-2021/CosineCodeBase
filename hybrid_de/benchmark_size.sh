#!/bin/sh
for i in 64 128 256 512 1024
do
    b="$(($i * 1024))"
    f="$((64 * 1024))"
    cargo run --release 2>/dev/null -- 4 1 0 1 $b $f
done