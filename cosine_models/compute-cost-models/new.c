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

double read_cost=0.0, avg_read_cost=0.0, update_cost=0.0, avg_update_cost=0.0;

int L = 0;
int FitsInMemory(double M, double data)
{
	//printf("%f has to fit inside %f\n", datadouble)E/(1024*1024*1024), M/(1024*1024*1024));
	if(M >= data*(double)E)
		return 1;
	else
		return 0;
}


int main(int argc, char* argv[]) {

    int T, K, Z, Y;
	double M, M_B, M_BF, M_BC, M_FP, M_F;
	double data = 10000000;
    B = 256*1024/(E);
    workload_type = atoi(argv[1]);
    double FPR_sum=1.0;

    if (workload_type == 0) 
    {
        printf("UNIFORM\n");
        U = 1000000000;
    }
    else
    {
        printf("SKEW\n");
        U_1 = 100000;
        U_2 = 100000000;
        p_put = 0.5;
        p_get = 0.5; //atof(argv[6]);
    }

    if(strcmp(argv[2], "rocks") == 0)
    {
        // run with ./verify 0 rocks 32 or ./verify 0 rocks 64 
        T = 16; 
        K = T/2;
        Z = K;
        Y = 0;
        M_B = atof(argv[3])*(1024*1024*1024); 
        M_BF = data * 10;
        M = M_B + M_BF; 
        M_FP = M - M_B;
        M_F = M - M_B;
        //double M_FP = M_B*F*T/(B*E);
        printf("M_B:%f, M_F:%f\n", M_B/(1024*1024*1024), M_F/(1024*1024*1024));
    }
    else if(strcmp(argv[2], "BDB") == 0)
    {
    	// run with ./verify 0 BDB 32 or ./verify 0 BDB 64 
    	T = 128; //defaultFanout in BDB
    	K = 1;
    	Z = 1;
    	M = atof(argv[3])*(1024*1024*1024); //B * E/ T;
    	M_B = M; //(double)M*(B*E)/(((double)B*E) + ((double)F*T));
    	M_BF = 0.0;
	    M_FP = M - M_B;
	    M_F = M - M_B;
    	//double M_FP = M_B*F*T/(B*E);
    	printf("M_B:%f, M_FP:%f\n", M_B/(1024*1024*1024), M_FP/(1024*1024*1024));
    }
    else if(strcmp(argv[2], "FASTER") == 0)
    {
        // run with ./verify 0 FASTER 32 or ./verify 0 FASTER 64 
        int scale_factor = 1000; // We assume about N/scale_factor keys fit in the in-memory hash table 
        M = atof(argv[3])*(1024*1024*1024); 
        M_F = ((double)data/scale_factor)*(F)*(1.0 + (1.0/B)); 
        M_B = M - M_F;
        T = (double)(data*E)/M_B; 
        K = T-1;
        Z = 0;
        M_BF = 0.0;
        M_BC = 0.0;
        Y = 0;
        L = 1;
        printf("M_F: %f, M_B: %f, T: %d\n", M_F/(1024*1024*1024), M_B/(1024*1024*1024), T);
    }
    else if(strcmp(argv[2], "FASTER_H") == 0)
    {
        // run with ./verify 0 FASTER 32 or ./verify 0 FASTER 64 
        int scale_factor = 1000; // We assume about N/scale_factor keys fit in the in-memory hash table 
        M = atof(argv[3])*(1024*1024*1024); 
        M_F = ((double)data/scale_factor)*(F)*(1.0 + (1.0/B)); 
        M_B = M - M_F;
        T = (double)(data*E)/M_B; 
        K = T-1;
        Z = -1;
        M_BF = 0.0;
        M_BC = 0.0;
        Y = 0;
        L = 1;
        printf("M_F: %f, M_B: %f, T: %d\n", M_F/(1024*1024*1024), M_B/(1024*1024*1024), T);
    }
    else if(strcmp(argv[2], "ANY") == 0)
    {
        // run with ./verify 0 ANY 32 or ./verify 0 ANY 33333333333 100000000000000 64 5 4 1 1 6.4 16.6 9.0 4
        if(argc > 3)
        {
            data = atof(argv[3]);
            U = atof(argv[4]);
            B = atof(argv[5]); 

            T = atoi(argv[6]); 
            K = atoi(argv[7]); 
            Z = atoi(argv[8]); 
            Y = atoi(argv[9]); 
            M_B = atof(argv[10])*(1024*1024*1024); 
            M_BF = atof(argv[11])*(1024*1024*1024); 
            M_FP = atof(argv[12])*(1024*1024*1024); 
            L = atoi(argv[13]); 
        }
        else
        {
            data = 33333333333;
            U = 100000000000000;
            B = 8*1024/(E);

            T = 5; 
            K = 4;
            Z = 1;
            Y = 1;
            M_B = 6.4*(1024*1024*1024); 
            M_BF = 16.6*(1024*1024*1024);
            M_FP = 9*(1024*1024*1024);
            L = 4;
            printf("M_B:%f, M_F:%f\n", M_B/(1024*1024*1024), M_F/(1024*1024*1024));
        }
        M = M_B + M_BF + M_FP; 
        M_F = M - M_B;

        analyzeUpdateCostAvgCase(&update_cost, T, K, Z, L, Y, M, M_F, M_B, data);
        analyzeReadCostAvgCase(&avg_read_cost, &FPR_sum, T, K, Z, L, Y, M, M_B, 0, M_F, M_BF, data);
        printf("read_cost: %f\n", avg_read_cost);
        printf("update_cost: %f\n", update_cost);
        return 0;
    }
	if(FitsInMemory(M_B, data))
	{
		printf("%f entries (%f GB) fits within %f GB.\n", data, (data*E)/(1024*1024*1024), M_B/(1024*1024*1024));
		exit(0);
	}
    if(strstr(argv[2], "FASTER") == NULL)
	//if(strcmp(argv[2], "FASTER") != 0 && strcmp(argv[2], "FASTER_H") != 0)
    {
        getNoOfLevelsAvgCase(&L, M_B, T, data); 
    }
    //getNoOfLevels(&L, M_B, T, data);
    
    printf("L: %d\n", L);
	analyzeUpdateCostAvgCase(&update_cost, T, K, Z, L, Y, M, M_F, M_B, data);
	analyzeReadCostAvgCase(&avg_read_cost, &FPR_sum, T, K, Z, L, Y, M, M_B, 0, M_F, M_BF, data);
	printf("Avg read_cost: %f Avg update_cost: %f\n", avg_read_cost, update_cost);
    update_cost = 0.0, avg_read_cost = 0.0;
    FPR_sum=1.0;
    analyzeUpdateCost(&update_cost, T, K, Z, L, Y, M, M_F, M_B);
    analyzeReadCost(&avg_read_cost, &FPR_sum, T, K, Z, L, Y, M, M_B, 0, M_F, M_BF, data);
    printf("Worst read_cost: %f Worst update_cost: %f\n", avg_read_cost, update_cost);
	return 0;
}
