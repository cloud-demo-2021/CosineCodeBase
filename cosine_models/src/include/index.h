#ifndef INDEX_GUARD
#define INDEX_GUARD

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>

//******************************* Beginning of variables ***************************

//******************************* End of variables ***************************

double monthlyPrice(double hourly_price);
void populateRAMParameters();
void getTotalNoOfDesigns(long int* td);
void setMaxRAMNeeded();
void initParameters();
int FitsInMemory(double M, double data);
void runSystem();
void setPricesBasedOnScheme();

//void runSystemForContinuum(double M, double q_count, double d_prop);

#endif
