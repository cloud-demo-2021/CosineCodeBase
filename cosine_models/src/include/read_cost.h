#pragma once 
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include "design_template.h"

bool consider_partial_last_level;
bool consider_partial_buffer;

void analyzeReadCost(double* read_cost, double* FPR_sum, int T, int K, int Z, int L, int Y, double M, double M_B, double M_BC, double M_F, double M_BF, double data);

void analyzeReadCostAvgCase(double* avg_read_cost, double* FPR_sum, int T, int K, int Z, int L, int Y, double M, double M_B, double M_BC, double M_F, double M_BF, double data);

void analyzeNoResultCost(double* no_result_read_cost, double* FPR_sum, int T, int K, int Z, int L, int Y, double M, double M_B, double M_BC, double M_F, double M_BF, double data);

void analyzeReadCostAvgCaseCPU(double* avg_read_cost_cpu, int T, int K, int Z, int L, int Y, double M, double M_B, double M_BC, double M_F, double M_BF);
void analyzeNoResultCPU(double* avg_read_cost_cpu, int T, int K, int Z, int L, int Y, double M, double M_B, double M_BC, double M_F, double M_BF);
double getAlpha_i(int type, double M_B, double M_BC, int T, int K, int Z, int L, int Y, int i, double data);
