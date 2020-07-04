#pragma once 
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>

#include "design_template.h"


double total_migration_time;
double stop_and_copy_time;
double dirty_rate;
int iterations;

void initMigration();
void getTotalMigrationTime(design** d_list, int iteration);