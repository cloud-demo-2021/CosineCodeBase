#!/bin/sh
for i in 2 4 8
do
	for j in 1 2 4 8 
    do 
        if [ $i -ge $j ]
        then 
            cargo run --release 2>/dev/null -- $i $j 
        fi
    done
done
