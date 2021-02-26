#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>

#include "include/long_scan_cost.h"
#include "include/environment_variable.h"
#include "include/workload.h"

void analyzeLongScanCost(double* long_scan_cost, int T, int K, int Z, int L, int Y, double M, double M_B, double M_F, double M_BF)
{
	//*long_scan_cost = (double)s*Z/(B); 
	if(Z <= 0) // LSH tables
	{
		*long_scan_cost = s;
	}
	else // non-LSH data structures
	{
		*long_scan_cost = (double)s/(B); 
	}
}

void analyzeScanCostAvgCase(double* long_scan_cost, int T, int K, int Z, int L, int Y, double M, double M_B, double M_F, double M_BF, double data)
{
	if(Z <= 0) // LSH tables
	{
		*long_scan_cost = s;
	}
	else if(T%16 == 0 && K==1) // B-trees
	{
		*long_scan_cost = L - 1 + s/B; 
	}
	else //LSM data structures
	{
		double selectivity = (double)s/data;
		double term1  = 0.0, term2 = 0.0, term3 = 0.0;
		double EB = M_B / ((double) E);
		for (int i = 1; i < L - Y + 1; i ++) {
			term1 += EB * pow(T, i);
		}
		if (T < B) {
			for (int i = L - Y + 1; i < L - 1; i++) {
				term2 += EB * pow(T, i);
			}
		}
		term3 = EB * pow(T, L);
		*long_scan_cost = (double)2*selectivity*(term1 + term2 + term3)/(B);
		if (T == B && Y > 0) {
			*long_scan_cost += Y - 1;
		} 
	}
}

void analyzeEmptyScanCost(double* long_scan_empty_cost, int T, int K, int Z, int L, int Y, double M, double M_B, double M_F, double M_BF, double data)
{
	/******************* Obtain FPR from sec 3.1 of Rosetta ******************/
	double R = 64;
	double FPR1 = ((double)N / pow(2, M_BF/(1.44 * data)))/U; // FPR for last level
	if(FPR1 > 1)
	{
		FPR1 = 1.0;
	}
	double FPR2 = (double)1.0 / (2 - FPR1); // FPR for all other levels
	if(!enable_Rosetta) // if Rosetta is disabled
	{
		FPR1 = 1.0;
		FPR2 = 1.0;
	}

	//printf("FPR1: %f, FPR2: %f for M_BF=%f data=%f\n", FPR1, FPR2, M_BF, data);
	*long_scan_empty_cost = (FPR1 * (double)Z * (Y+1)) + (FPR2 * (double)K * (L-Y-1));
}