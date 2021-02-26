#ifndef CONTINUUM_GUARD
#define CONTINUUM_GUARD

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>

//******************************* Beginning of variables ***************************

typedef struct continuum_design
{
	int T;
	int K;
	int Z;
	int L;
	int Y;
	int C;
	double M, M_B, M_BC, M_F, M_FP, M_BF, FPR_sum;
	double update_cost, read_cost, rmw_cost, blind_update_cost, short_scan_cost, long_scan_cost, long_scan_empty_cost, total_cost;
	double data_VM, workload_VM;
	int compression_id;
	char* msg;
}continuum_design;


typedef struct continuum_node
{
	int provider_id;
	int* VM_arr;
	struct continuum_design** VM_design_arr;
	double mem_sum;
	double total_IO;
	double storage_cost, SLA_cost, cost;
	struct continuum_node* next;
	struct continuum_node* down;
	double latency;
	int no_of_instances;
	int compression_id;
}continuum_node;

continuum_node* p_continuum;
int no_of_continuum_node;

//******************************* End of variables ***************************

void buildContinuum();
void evaluateConfiguration(int provider_id, int* VM_arr, double mem_sum, double monthly_storage_cost, double SLA_cost, int compression_id);
continuum_node* newContinuumNode(int provider_id, int* VM_arr, double mem_sum, int compression_id);
void addContinuumNode(continuum_node* node);
void printContinuumNode(continuum_node* node);
void printContinuum(bool entire);
void correctContinuum();
void correctContinuumForEachProvider();
void updateContinuumNode(continuum_node* node_A, continuum_node* node_B);
continuum_node* findBest(continuum_node* node);
void printContinuumAtGap();

#endif
