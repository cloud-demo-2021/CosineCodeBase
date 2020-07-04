#ifndef COMPARE_DESIGNS_GUARD
#define COMPARE_DESIGNS_GUARD

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>

#include"design_template.h"

// ****************************** VARIABLES ******************************

double w_tiered_LSM_cost;
double w_lazy_leveled_LSM_cost;
double w_leveled_LSM_cost;
double w_COLA_cost;
double w_b_plus_tree_cost;
double w_b_epsilon_tree_cost;

double r_tiered_LSM_cost;
double r_lazy_leveled_LSM_cost;
double r_leveled_LSM_cost;
double r_COLA_cost;
double r_b_plus_tree_cost;
double r_b_epsilon_tree_cost;

double tiered_LSM_cost;
double lazy_leveled_LSM_cost;
double leveled_LSM_cost;
double COLA_cost;
double b_plus_tree_cost;
double b_epsilon_tree_cost;

//******************************* FUNCTIONS ***************************

void getTieredLSMCost(design* d);
void getLazyLeveledLSMCost(design* d);
void getLeveledLSMCost(design* d);
void getCOLACost(design* d);
void getBEpsilonTreeCost(design* d);
void getBPlusTreeCost(design* d);
void compareWithOtherDesigns(design* d);

//******************************* End of ENVIRONMENTAL PARAMETERS ***************************

#endif
