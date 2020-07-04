#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>

#include "include/short_scan_cost.h"
#include "include/environment_variable.h"

void analyzeShortScanCost(double* short_scan_cost, int T, int K, int Z, int L, int Y, double M, double M_B, double M_F, double M_BF)
{
	if(Y == 0)
	{
		*short_scan_cost = (double)K*L; 
	}
	else
	{
		*short_scan_cost = (double)K*(L-Y-1) + (double)Z*(Y+1); 
	}
}