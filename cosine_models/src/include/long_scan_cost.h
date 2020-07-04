#pragma once 
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include "design_template.h"


void analyzeLongScanCost(double* long_scan_cost, int T, int K, int Z, int L, int Y, double M, double M_B, double M_F, double M_BF);
void analyzeScanCostAvgCase(double* scan_cost, int T, int K, int Z, int L, int Y, double M, double M_B, double M_F, double M_BF);