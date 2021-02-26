#pragma once
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include "design_template.h"
#include "continuum.h"


void getNoOfLevels(int* L, double M_B, int T, double data);
void getNoOfLevelsAvgCase(int* L, double M_B, int T, double data);
void getX(double* X, int T, int K, int Z);
void getY(int* Y, double* M_FP, double M_F, double M_F_HI, double M_F_LO, double X, int T, int L, double data);
void getM_FP(double* M_FP, int T, int L, int Y, double M_B, double M_F_LO);
void getM_BF(double* M_BF, double M_F, double M_FP);
int validateFilterMemoryLevels(double M_F, double M_F_HI, double M_F_LO);
double min(double a, double b);
void getM_F_LO(double* M_F_LO, double *M_B, double M, int T, double data);
void navigateDesignSpace1(double M, double M_B, design** d_list, char* mode);
void set_M_B(double* M_F, double* M_B, double M, double M_F_HI, double M_F_LO);
void set_M_F(double* M_F, double* M_B, double M, double M_F_HI, double M_F_LO);

void navigateDesignSpaceForContinuumSingleMachine(double M, double data, double workload, continuum_design** bucket, int compression_id);
int getCostForExistingSystemsDefaultDesign(double M, double data, double workload, continuum_design** bucket, double cost, int provider_id, int compression_id);
int getCostForExistingSystemsTunedDesign(double M, double data, double workload, continuum_design** bucket, double cost, int provider_id, int compression_id);


