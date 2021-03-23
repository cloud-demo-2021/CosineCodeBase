#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include <string.h>

#include "../src/include/environment_variable.h"
#include "../src/update_cost.c"
#include "../src/read_cost.c"
#include "../src/include/workload.h"
#include "../src/parameter_computation.c"
#include "../src/compression_library.c"

double read_cost=0.0, avg_read_cost=0.0, avg_cpu_cost=0.0,update_cost=0.0, avg_update_cost=0.0,no_result_cost=0.0, no_result_cpu=0.0, short_scan_cost=0.0, long_scan_cost=0.0;

int L = 0;
int FitsInMemory(double M, double data)
{
	//printf("%f has to fit inside %f\n", datadouble)E/(1024*1024*1024), M/(1024*1024*1024));
	if(M >= data*(double)E)
		return 1;
	else
		return 0;
}

void computeCosts(double M, double M_B, double M_BC, double M_BF, int T, int K, int Z, int Y, double data, char* system)
{
	double M_F = M - M_B; // = ((B*E + (M - M_F)) > 0 ? B*E + (M - M_F) : (B*E)); // byte
	double FPR_sum=1.0;

	getNoOfLevelsAvgCase(&L, M_B, T, data);
	printf("L avg case: %d\n", L);
	if(strcmp(system, "WT") == 0)
	{
		Y = L - 1;
	}

	if(strcmp(system, "BDB") == 0)
	{
		Y = L - 1;
	}
	
	//printf("EB: %f\n", (double)M_B/E);
	//printf("System is %s\n", system);

	// get costs 	
	analyzeUpdateCostAvgCase(&avg_update_cost, T, K, Z, L, Y, M, M_F, M_B);

	printf("avg case update:%f\n", avg_update_cost);

	analyzeReadCostAvgCase(&avg_read_cost, &FPR_sum, T, K, Z, L, Y, M, M_B, M_BC, M_F, M_BF, data);
	printf("avg case read:%f\n", avg_read_cost);

	printf("FPR_sum:%f\n", FPR_sum);

	getNoOfLevels(&L, M_B, T, data);
	printf("L normal: %d\n", L);

	analyzeUpdateCost(&update_cost, T, K, Z, L, Y, M, M_F, M_B);
	analyzeReadCost(&read_cost, &FPR_sum, T, K, Z, L, Y, M, M_B, M_BC, M_F, M_BF, data);

	printf("FPR_sum:%f\n", FPR_sum);
	printf("worst case update:%f\n", update_cost);
	printf("worst case read:%f\n", read_cost);
}


int main(int argc, char* argv[]) {

    int T, K, Z, Y;
	double M, M_B, M_BF, M_BC;
	double data = 10000000;

    workload_type = atoi(argv[1]);
    //double data = atof(argv[3]);

    B = 4096 / E; // set environment variable

    if(strcmp(argv[2], "rocks") == 0)
    {
    	// run with ./verify 0 rocks 32 or ./verify 0 rocks 64 
    	printf("wheee\n");
    	T = 10;
    	K = 1;
    	Z = 1;
    	Y = 0;
    	M =  150000000;
    	M_BC = 0.0;
    	M_BF = data * 14.0/8.0; // 10 bits/entry in RocksDB is default and convert to byte because everything else is in byte
    	M_B = 67000000;
  		printf("M_BF: %f, M_B: %f\n", M_BF/(1024*1024*1024), M_B/(1024*1024*1024));
    	if(M_B < 0)
    	{
    		M_B = 0;
    	}
    	if (workload_type == 0) 
    	{
    		U = 100000000;
    	}
    	else
    	{
    		U_1 = 15000;
			U_2 = 100000000;
	    	p_put = atof(argv[5]);
	    	// data = (data)/ (1 - p_put);
	    	p_get = atof(argv[6]);
	    	printf("p_put:%f, p_get:%f\n", p_put, p_get);
    	}
    }
    else if(strcmp(argv[2], "WT") == 0)
    {
    	// run with ./verify 0 WT 32 or ./verify 0 WT 64 
    	T = 32; //defaultFanout in WT
    	K = 1;
    	Z = 1;
    	//Y = 0;
    	M_B = B * E/ T;
    	printf("M_B:%f\n", M_B);
    	double M_FP = 0.0; //M_B*F*T/(B*E);
    	M_BC = 0;
    	M =  M_BC;
    	// M_BC = 0;
    	M_BF = 0.0; // No BF in WT
    	if (workload_type == 0) 
    	{
    		U = 100000000;
    	}
    	else
    	{
    		U_1 = 50000;
			U_2 = 100000000;
	    	p_put = atof(argv[5]);
	    	p_get = atof(argv[6]);
	    	// data = (data)/ (1 - p_put);
	    	// printf("data:%f\n", data);
    	}
    }
    else if(strcmp(argv[2], "BDB") == 0)
    {
    	// run with ./verify 0 BDB 32 or ./verify 0 BDB 64 
    	T = 128; //defaultFanout in BDB
    	K = 1;
    	Z = 1;
    	//Y = 0;
    	M = atof(argv[3])*(1024*1024*1024); //B * E/ T;
    	M_B = (double)M*(B*E)/(((double)B*E) + ((double)F*T));
    	double M_FP = M_B*F*T/(B*E);
    	printf("M_B:%f, M_FP:%f\n", M_B/(1024*1024*1024), M_FP/(1024*1024*1024));
    	M_BC = 0;
    	M_BF = 0.0; // No BF in BDB
    	if (workload_type == 0) 
    	{
    		U = 100000000;
    	}
    	else
    	{
    		U_1 = 100000;
			U_2 = 100000000;
	    	p_put = atof(argv[5]);
	    	p_get = atof(argv[6]);
	    	// data = (data)/ (1 - p_put);
	    	// printf("data:%f\n", data);
    	}
    }
    else
    {
    	int expected_num = 11;
	    if (workload_type == 1) expected_num = 14;
	    if (argc != expected_num) {
	      fprintf(stderr, "Usage:<workload_type>, <U>, <T>, <K>, <Z>, <Y>, <M>, <MB>, <MBF>, <MBC>");
	      exit(1);
	    }

		T = atoi(argv[2]);
		K = atoi(argv[3]);
		Z = atoi(argv[4]);
		Y = atoi(argv[5]);
		M = atof(argv[6]) * 1024 * 1024 *1024;
		M_B = atof(argv[7]) * 1024 * 1024 *1024;
		M_BF = atof(argv[8]) * 1024 * 1024 *1024;
		M_BC = atof(argv[9]) * 1024 * 1024 *1024;

		if (workload_type == 0) {
			U = 10000000000;
			printf("U: %f\n", U);
		}
		else {
			U_1 = atof(argv[10]);
			U_2 = atof(argv[11]);
	    	p_put = atof(argv[12]);
	    	p_get = atof(argv[13]);
	    	// printf("U_1:%f, U_2:%f, p_put:%f, p_get:%f\n", U_1, U_2, p_put, p_get);

		}
    }
	if(FitsInMemory(M_B, data))
	{
		printf("%f entries (%f GB) fits within %f GB.\n", data, (data*E)/(1024*1024*1024), M_B/(1024*1024*1024));
		exit(0);
	}
	computeCosts(M, M_B, M_BC, M_BF, T, K, Z, Y, data, argv[2]);
	printf("Design: N:%f, T: %d, K: %d, Z:%d, Y:%d, L:%d, M:%f, M_B:%f, M_BF: %f, M_BC: %f, E:%d\n", data, T, K, Z, Y, L, M/(1024*1024*1024), M_B/(1024*1024*1024), M_BF/(1024*1024*1024), M_BC/(1024*1024*1024), E);
	printf("update cost: %f, read_cost: %f, avg_read_cost: %f", update_cost, read_cost, avg_read_cost);
	return 0;
}
