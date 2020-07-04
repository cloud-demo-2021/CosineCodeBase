#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>

#include "../src/include/environment_variable.h"
#include "../src/update_cost.c"
#include "../src/read_cost.c"
#include "../src/include/workload.h"
#include "../src/parameter_computation.c"

double read_cost=0.0, avg_read_cost=0.0, avg_cpu_cost=0.0,update_cost=0.0, avg_update_cost=0.0,no_result_cost=0.0, no_result_cpu=0.0, short_scan_cost=0.0, long_scan_cost=0.0;

int L = 0;


void computeCosts(double M, double M_B, double M_BC, double M_BF, int T, int K, int Z, int Y, double data)
{
	double M_F = M - M_B; // = ((B*E + (M - M_F)) > 0 ? B*E + (M - M_F) : (B*E)); // byte
	double FPR_sum=1.0;

	getNoOfLevelsAvgCase(&L, M_B, T, data);
	printf("L avg case: %d\n", L);


	// get costs 	
	analyzeUpdateCostAvgCase(&avg_update_cost, T, K, Z, L, Y, M, M_F, M_B);

	analyzeReadCostAvgCase(&avg_read_cost, &FPR_sum, T, K, Z, L, Y, M, M_B, M_BC, M_F, M_BF, data);
	analyzeReadCostAvgCaseCPU(&avg_cpu_cost, T, K, Z, L, Y, M, M_B, M_BC, M_F, M_BF);

	printf("FPR_sum:%f\n", FPR_sum);

	analyzeNoResultCost(&no_result_cost, &FPR_sum, T, K, Z, L, Y, M, M_B, M_BC, M_F, M_BF, data);	
	analyzeNoResultCPU(&no_result_cpu, T, K, Z, L, Y, M, M_B, M_BC, M_F, M_BF);

	getNoOfLevels(&L, M_B, T, data);
	printf("L normal: %d\n", L);

	analyzeUpdateCost(&update_cost, T, K, Z, L, Y, M, M_F, M_B);
	analyzeReadCost(&read_cost, &FPR_sum, T, K, Z, L, Y, M, M_B, M_BC, M_F, M_BF, data);

	printf("FPR_sum:%f\n", FPR_sum);


	printf("total cost average:%f\n", (query_count) * (read_percentage * avg_read_cost /100 + write_percentage * avg_update_cost /100));

	printf("total cost worst:%f\n", (query_count) * (read_percentage * read_cost /100 + write_percentage * update_cost /100));


	printf("EB: %f\n", M_B/E);
}


int main(int argc, char* argv[]) {
	if (argc < 2) {
      fprintf(stderr, "Usage:<workload_type>, <T>, <K>, <Z>, <Y>, <M>, <MB>, <MBF>, <MBC>, <U or U_1,U_2,p_put,p_get>");
      exit(1);
    }
    workload_type = atoi(argv[1]);
    int expected_num = 11;
    if (workload_type == 1) expected_num = 14;
    if (argc != expected_num) {
      fprintf(stderr, "Usage:<workload_type>, <U>, <T>, <K>, <Z>, <Y>, <M>, <MB>, <MBF>, <MBC>");
      exit(1);
    }	

	int T = atoi(argv[2]);
	int K = atoi(argv[3]);
	int Z = atoi(argv[4]);
	int Y = atoi(argv[5]);
	double M = atof(argv[6]) * 1000000000;
	double M_B = atof(argv[7]) * 1000000000;
	double M_BF = atof(argv[8]) * 1000000000;
	double M_BC = atof(argv[9]) * 1000000000;
	// double data = 10000000;

	if (workload_type == 0) {
		U = atof(argv[10]);
		printf("U: %f\n", U);
	}
	else {
		U_1 = atof(argv[10]);
		U_2 = atof(argv[11]);
    	p_put = atof(argv[12]);
    	p_get = atof(argv[13]);
    	printf("U_1:%f, U_2:%f, p_put:%f, p_get:%f\n", U_1, U_2, p_put, p_get);

	}

	B = 4096 / E; // set environment variable
	printf("B:%d\n", B);
	double data = 10000000;
	computeCosts(M, M_B, M_BC, M_BF, T, K, Z, Y, data);
	printf("Design: N:%f, T: %d, K: %d, Z:%d, Y:%d, L:%d, M:%f, M_B:%f, M_BC: %f, E:%d\n", data, T, K, Z, Y, L, M, M_B, M_BC, E);
	// printf("update cost: %f, read_cost: %f, avg_read_cost: %f, avg_cpu_cost:%f, no_result_cost:%f, no_result_cpu:%f", update_cost, read_cost, avg_read_cost, avg_cpu_cost, no_result_cost, no_result_cpu);
	printf("update cost: %f, avg_update_cost: %f, read_cost: %f, avg_read_cost: %f, avg_cpu_cost:%f", update_cost, avg_update_cost, read_cost, avg_read_cost, avg_cpu_cost);

	return 0;
}