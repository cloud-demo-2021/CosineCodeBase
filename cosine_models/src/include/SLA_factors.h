#ifndef SLA_FACTOR_GUARD
#define SLA_FACTOR_GUARD

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include <stdbool.h> 

// *********************************************************************************
// ****************************** Beginning of variables ***************************
// *********************************************************************************

double DB_migration_cost_AWS;
double DB_migration_cost_GCP;
double DB_migration_cost_Azure;
static bool enable_DB_migration = true;

double dev_ops_AWS;
double dev_ops_GCP;
double dev_ops_Azure;
static bool enable_dev_ops = true;

double backup_AWS;
double backup_GCP;
double backup_Azure;
static bool enable_backup = true;

//******************************* End of variables ***************************

void initializeSLAFactors();
void computeSLARelatedCost(int cloud_provider, double* SLA_cost);

#endif
