#!/bin/sh
for i in 2 3 4 5 6 7 8 9
do
	cargo run --release 2>/dev/null -- $i $i
done