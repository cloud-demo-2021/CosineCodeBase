#pragma once
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include "design_template.h"



void analyzeUpdateCost(double* update_cost, int T, int K, int Z, int L, int Y, double M, double M_F, double M_B);

void analyzeUpdateCostAvgCase(double* update_cost, int T, int K, int Z, int L, int Y, double M, double M_F, double M_B, double data, int C);
