#ifndef EXPERIMENT_GUARD
#define EXPERIMENT_GUARD

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include <stdbool.h> 

void printContinuumExistingSystem();
void printContinuumCosine(bool entire);
void getBestAndWorstCase(int provider_id, int T, int K, int Z, int L, int Y, double M, double M_F, double M_B, double M_BF, double data, double workload_VM);

#endif
