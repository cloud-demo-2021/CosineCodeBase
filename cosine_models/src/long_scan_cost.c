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
	*long_scan_cost = (double)s/(B); 
}

void analyzeScanCostAvgCase(double* long_scan_cost, int T, int K, int Z, int L, int Y, double M, double M_B, double M_F, double M_BF)
{
	double selectivity = (double)s/N;
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