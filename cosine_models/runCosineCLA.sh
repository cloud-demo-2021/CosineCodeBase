#!/bin/bash

EXEC_FILE="./build/a.out"

CLOUD_PROVIDER="AWS"
BUDGET=20000
DATA_ENTRIES=1000000000000
WL_TYPE=1
GET_PCT=50
PUT_PCT=50
SCAN_PCT=0
QUERY_COUNT=10000000000
U=100000000000000
U1=100000000000000
U2=100000000000
P_PUT=0.1
P_GET=0.0001
AVG=$(echo $(echo $P_PUT + $P_GET | bc -l) / 2 | bc -l)



if [ $WL_TYPE -eq 0 ]
then
	DATA_PCT=50
	GET_QUERY_PCT=100	
	PUT_QUERY_PCT=10

	command="$EXEC_FILE $CLOUD_PROVIDER $BUDGET $DATA_ENTRIES $WL_TYPE $GET_PCT $PUT_PCT $SCAN_PCT $QUERY_COUNT $U" # Uniform distribution
	echo "\n =========================== Running command $command =========================== \n"
	eval $command
	echo "\n =========================== Finished command $command =========================== \n"

	BUDGET1=$(echo $DATA_PCT \* $BUDGET / 100| bc)
	DATA_ENTRIES1=$(echo $DATA_PCT \* $DATA_ENTRIES /100 | bc)
	GET_QUERY=$(echo $GET_QUERY_PCT \* $GET_PCT \* $QUERY_COUNT / 10000 | bc)
	PUT_QUERY=$(echo $PUT_QUERY_PCT \* $PUT_PCT \* $QUERY_COUNT / 10000 | bc)
	# echo $GET_QUERY
	# echo $PUT_QUERY
	QUERY_COUNT1=$(echo $GET_QUERY + $PUT_QUERY | bc)
	GET_PCT1=$(echo $GET_QUERY \* 100 / $QUERY_COUNT1 | bc)
	PUT_PCT1=$(echo $PUT_QUERY \* 100 / $QUERY_COUNT1 | bc)

	command="$EXEC_FILE $CLOUD_PROVIDER $BUDGET1 $DATA_ENTRIES1 $WL_TYPE $GET_PCT1 $PUT_PCT1 $SCAN_PCT $QUERY_COUNT1 $U"
	echo "\n =========================== Running command $command =========================== \n"
	eval $command
	echo "\n =========================== Finished command $command =========================== \n"

	BUDGET2=$(echo $(echo 100 - $DATA_PCT | bc) \* $BUDGET / 100 | bc)
	DATA_ENTRIES2=$(echo $(echo 100 - $DATA_PCT | bc) \* $DATA_ENTRIES / 100 | bc)
	GET_QUERY=$(echo $(echo 100 - $GET_QUERY_PCT | bc -l) \* $GET_PCT \* $QUERY_COUNT / 10000 | bc)
	PUT_QUERY=$(echo $(echo 100 - $PUT_QUERY_PCT | bc -l) \* $PUT_PCT \* $QUERY_COUNT / 10000 | bc)
	# echo $GET_QUERY
	# echo $PUT_QUERY
	QUERY_COUNT2=$(echo $GET_QUERY + $PUT_QUERY | bc)
	GET_PCT2=$(echo $GET_QUERY \* 100 / $QUERY_COUNT2 | bc)
	PUT_PCT2=$(echo $PUT_QUERY \* 100 / $QUERY_COUNT2 | bc)

	command="$EXEC_FILE $CLOUD_PROVIDER $BUDGET2 $DATA_ENTRIES2 $WL_TYPE $GET_PCT2 $PUT_PCT2 $SCAN_PCT $QUERY_COUNT2 $U"
	echo "\n =========================== Running command $command =========================== \n"
	eval $command
	echo "\n =========================== Finished command $command =========================== \n"
else
	command="$EXEC_FILE $CLOUD_PROVIDER $BUDGET $DATA_ENTRIES $WL_TYPE $GET_PCT $PUT_PCT $SCAN_PCT $QUERY_COUNT $U1 $U2 $P_PUT $P_GET" # Skew distribution
	echo "\n =========================== Running command $command =========================== \n"
	eval $command
	echo "\n =========================== Finished command $command =========================== \n"

	BUDGET1=$(echo $P_PUT \* $BUDGET | bc)
	DATA_ENTRIES1=$(echo $P_PUT \* $DATA_ENTRIES | bc)
	GET_QUERY=$(echo $P_GET \* $GET_PCT \* $QUERY_COUNT / 100 | bc)
	PUT_QUERY=$(echo $P_PUT \* $PUT_PCT \* $QUERY_COUNT / 100 | bc)
	# echo $GET_QUERY
	# echo $PUT_QUERY
	QUERY_COUNT1=$(echo $GET_QUERY + $PUT_QUERY | bc)
	GET_PCT1=$(echo $GET_QUERY \* 100 / $QUERY_COUNT1 | bc)
	PUT_PCT1=$(echo $PUT_QUERY \* 100 / $QUERY_COUNT1 | bc)

	command="$EXEC_FILE $CLOUD_PROVIDER $BUDGET1 $DATA_ENTRIES1 0 $GET_PCT1 $PUT_PCT1 $SCAN_PCT $QUERY_COUNT1 $U1"
	echo "\n =========================== Running command $command =========================== \n"
	eval $command
	echo "\n =========================== Finished command $command =========================== \n"

	BUDGET2=$(echo $BUDGET - $BUDGET1 | bc)
	DATA_ENTRIES2=$(echo $(echo 1 - $P_PUT | bc) \* $DATA_ENTRIES | bc)
	GET_QUERY=$(echo $(echo 1 - $P_GET | bc -l) \* $GET_PCT \* $QUERY_COUNT / 100 | bc)
	PUT_QUERY=$(echo $(echo 1 - $P_PUT | bc -l) \* $PUT_PCT \* $QUERY_COUNT / 100 | bc)
	# echo $GET_QUERY
	# echo $PUT_QUERY
	QUERY_COUNT2=$(echo $GET_QUERY + $PUT_QUERY | bc)
	GET_PCT2=$(echo $GET_QUERY \* 100 / $QUERY_COUNT2 | bc)
	PUT_PCT2=$(echo $PUT_QUERY \* 100 / $QUERY_COUNT2 | bc)

	command="$EXEC_FILE $CLOUD_PROVIDER $BUDGET2 $DATA_ENTRIES2 0 $GET_PCT2 $PUT_PCT2 $SCAN_PCT $QUERY_COUNT2 $U2"
	echo "\n =========================== Running command $command =========================== \n"
	eval $command
	echo "\n =========================== Finished command $command =========================== \n"
fi
exit 1


