#ifndef WORKLOAD_GUARD
#define WORKLOAD_GUARD

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include "design_template.h"

// *********************************************************************************
//******************************* Beginning of variables ***************************
// *********************************************************************************



//******************************* Continuum variables ***************************
typedef struct continuum
{
	double M;
	double update_cost;
	double price;
	struct continuum* next;
} continuum;

continuum* head;

//******************************* Workload window variables ***************************
typedef struct window
{
	double read_percentage, write_percentage;
	long int query_count;
}window;

window* windows;
int no_of_windows;
int change_percent;
double workload_exec_time, act_exec_time;

//******************************* Workload composition variables ***************************
// Need to refactor this.

// for Uniform
double U; //= 100000000000000;
double p_put; // = 0.8; // fraction of the time that you call get on elements in U_1
double U_1; //= 100000;
double U_2; // = 1000000000000;
// NOTE: it must always be true that (p_put / U_1) > (1 / U_2)
double p_get; // = 0.8; // fraction of the time that you call put on elements in U_1
int workload_type; // = 0; // 0 is uniform, 1 is skew

double read_percentage; // = 40;
double write_percentage; // = 60;
double short_scan_percentage; // = 0;
double long_scan_percentage; // = 0;

long int query_count; // = 4000000000;
int s; //= 4096; // in entries, for long scan queries

//******************************* Resource and design variables ***************************
double max_RAM_needed; // in GB
design* d_list;

// *********************************************************************************
//******************************* End of variables *********************************
// *********************************************************************************

// *********************************************************************************
//******************************* Beginning of functions ***************************
// *********************************************************************************

void initWorkload();
void getM_B(double* M_B, int T, int L);
void generateWorkloadOverTime();
void showWorkloadWindows();
void setOptimalStaticDesign();

#endif
