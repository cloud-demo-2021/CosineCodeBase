#!/bin/bash

BASE_DIR="/Users/subarnachatterjee/Dropbox/cloud_continuum/cloud_cost_code_revised"
EXEC_FILE="./build/a.out"
RESULT_FILE="$BASE_DIR/data.txt"
rm $RESULT_FILE

BUDGET=20000
BASELINE_ARRAY=("rocks" "WT" "FASTER_H")
DATA_ENTRIES=1000000000000
WL_TYPE=0
GET_PCT=10
INSERT_PCT=25
RMW_PCT=5
BLIND_PCT=30
SCAN_PCT=10
SCAN_EMPTY_PCT=20
QUERY_COUNT=10000000000
U=100000000000000
U1=100000000000000
U2=100000000000
P_PUT=0.2
P_GET=0.05
AVG=$(echo $(echo $P_PUT + $P_GET | bc -l) / 2 | bc -l)
RUNNING_COSINE=0

DATA_ARR=(100000000000 500000000000 1000000000000 5000000000000 10000000000000 50000000000000 100000000000000) #10^11 - 10^14 i.e., 0.1T 0.5T 1T 5T 10T


if [ $RUNNING_COSINE -eq 1 ]
then
	echo "RUNNING COSINE\n"
	for DATA in "${DATA_ARR[@]}"; do
		command="$EXEC_FILE $DATA $QUERY_COUNT $GET_PCT $INSERT_PCT $RMW_PCT $BLIND_PCT $SCAN_PCT $SCAN_EMPTY_PCT $WL_TYPE $U $U1 $U2 $P_PUT $P_GET >> $RESULT_FILE" # Uniform distribution
		echo "\n =========================== Running command $command =========================== \n"
		eval $command
		echo "\n =========================== Finished command $command =========================== \n"
	done

	##### use only for experiments where data and budget grow simulatneously! ########
	cat $RESULT_FILE | grep -m1 ^20000.0 | tail -n1
	cat $RESULT_FILE | grep -m2 ^25000.0 | tail -n1
	cat $RESULT_FILE | grep -m3 ^30000.0 | tail -n1
	cat $RESULT_FILE | grep -m4 ^35000.0 | tail -n1
	cat $RESULT_FILE | grep -m5 ^40000.0 | tail -n1
	cat $RESULT_FILE | grep -m6 ^45000.0 | tail -n1
	cat $RESULT_FILE | grep -m7 ^50000.0 | tail -n1
	##### ###### ###### ###### ###### ###### ###### ###### ###### ###### ###### ###### ###### 
fi


# for DATA in "${DATA_ARR[@]}"; do
# 	command="$EXEC_FILE $DATA $QUERY_COUNT $GET_PCT $INSERT_PCT $RMW_PCT $BLIND_PCT $SCAN_PCT $SCAN_EMPTY_PCT $WL_TYPE $U $U1 $U2 $P_PUT $P_GET $BASELINE >> $RESULT_FILE" # Uniform distribution
# 	echo "\n =========================== Running command $command =========================== \n"
# 	eval $command
# 	echo "\n =========================== Finished command $command =========================== \n"
# done


###########################################################################################################################################################################
################################################################  RUN FOR BASELINES ONLY ###################################################################################
############################################################################################################################################################################# 
if [ $RUNNING_COSINE -eq 0 ]
then
	echo "RUNNING BASELINES\n"
	cd compute-cost-models
	MEMORY=1920.0 #3840.0 #9216.0
	gcc -o run new_tuned_opportunities.c
	for BASELINE in "${BASELINE_ARRAY[@]}"; do
		BASELINE_RESULT_FILE="$BASE_DIR/$BASELINE.txt"
		rm $RESULT_FILE $BASELINE_RESULT_FILE
		for DATA in "${DATA_ARR[@]}"; do

			###### use only for experiments where data and budget grow simulatneously! ########
			if [ $DATA -eq 100000000000 ]
			then
				MEMORY=1920.0
				echo Memory: $MEMORY.
			elif [ $DATA == 500000000000 ]; then
				MEMORY=3200.0
				echo Memory: $MEMORY.
			elif [ $DATA -gt 500000000000 ] && [ $DATA -lt 100000000000000 ]; then
				MEMORY=3840.0
				echo Memory: $MEMORY.
			else
				MEMORY=9216.0
				echo Memory: $MEMORY.
			fi
			###### ###### ###### ###### ###### ###### ###### ###### ###### ###### ###### ###### ###### 

			command="./run $WL_TYPE $BASELINE $MEMORY $DATA >> $BASELINE_RESULT_FILE" # Uniform distribution
			echo "\n =========================== Running command $command =========================== \n"
			eval $command
			echo "\n =========================== Finished command $command =========================== \n"
		done
	done
	for BASELINE in "${BASELINE_ARRAY[@]}"; do
		BASELINE_RESULT_FILE="$BASE_DIR/$BASELINE.txt"
		cat $BASELINE_RESULT_FILE | grep speedup | cut -f2 -d: | cut -f2 -d' ' >> $RESULT_FILE
		echo "\n" >> $RESULT_FILE
	done

	cat $RESULT_FILE
fi
############################################################################################################################################################################
################################################################ BASELINES END ###################################################################################
############################################################################################################################################################################# 

