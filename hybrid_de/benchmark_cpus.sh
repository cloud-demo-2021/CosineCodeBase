#!/bin/sh
# for i in 1 2 4 6 8 10
# do
# 	cargo run --release 2>/dev/null -- 10 $j
# done

for i in 1 2 4 8 16
do
    for j in 1 2 3
    do 
	    cargo run --release 2>/dev/null -- $i
    done
done