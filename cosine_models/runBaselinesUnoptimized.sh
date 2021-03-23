#!/bin/bash


baseline='FASTER_H'
result_filename='data.txt'
baseline_filename=$baseline'.txt'
distribution=0 # 0 for uniform 1 for skew

rm $result_filename
rm compute-cost-models/$baseline_filename
./build/a.out $baseline > $result_filename
start=$(cat $result_filename | grep -n "Continuum built" | cut -d: -f1)
#start=$((start+1))
echo "\nstarting to read from line" $start "of" $result_filename

cnt=1
arr_budget=()
arr_performance=()
while read LINE
do
    if [ "$cnt" -gt $start ];
    then
    	budget=$(echo $LINE | cut -f1 -d' ')
    	memory=$(echo $LINE | cut -f2 -d' ')
    	arr_budget+=($budget)
    	#echo $budget "\t" $memory
        #echo $LINE | cut -f1 -d' '

        cd compute-cost-models
        ./verify $distribution $baseline $memory >> $baseline_filename
        result_set=$(./verify $distribution $baseline $memory)
        result_set=$(echo $result_set | cut -f13 -d:)
        result_set=$(echo $result_set | cut -f1 -d' ')
        #echo $result_set
        arr_performance+=($result_set)
        #./verify 0 rocks $memory >> $baseline_filename
        cd ..
    fi
    cnt=$((cnt+1))
done < $result_filename



for ((idx=0; idx<${#arr_budget[@]}; ++idx)); do
    echo "$idx\t" "${arr_budget[idx]}\t" "${arr_performance[idx]}"
done