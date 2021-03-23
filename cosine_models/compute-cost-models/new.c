#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include <string.h>

#include "../src/include/environment_variable.h"
#include "../src/update_cost.c"
#include "../src/read_cost.c"
#include "../src/long_scan_cost.c"
#include "../src/include/workload.h"
#include "../src/parameter_computation.c"
#include "../src/compression_library.c"
#include "../src/include/parallelism.h"
#include "../src/include/VM_library.h"

double read_cost=0.0, avg_read_cost=0.0, update_cost=0.0, avg_update_cost=0.0, blind_update_cost=0.0, rmw_cost=0.0, long_scan_cost=0.0, long_scan_empty_cost=0.0;

int L = 0;
double overall_prop_parallelizable_code = 0.0;

int FitsInMemory(double M, double data)
{
	//printf("%f has to fit inside %f\n", datadouble)E/(1024*1024*1024), M/(1024*1024*1024));
	if(M >= data*(double)E)
		return 1;
	else
		return 0;
}

void setOverallProportionOfParallelizableCode(char* existing_system)
{
    if(enable_parallelism)
    {
        if(existing_system)
        {
            if(strcmp(existing_system, "rocks") == 0) // RocksDB
            {
                overall_prop_parallelizable_code = ((double)(read_percentage + rmw_percentage  + long_scan_percentage + long_scan_empty_percentage)*prop_of_parallelizable_code_reads_rocks/100) + ((double)(write_percentage + blind_update_percentage)*prop_of_parallelizable_code_writes_rocks/100);
            }
            else if(strstr(existing_system, "FASTER") != NULL) // Any FASTER versions
            {
                overall_prop_parallelizable_code = ((double)(read_percentage + rmw_percentage + blind_update_percentage + long_scan_percentage + long_scan_empty_percentage)*prop_of_parallelizable_code_reads_FASTER/100) + ((double)write_percentage*prop_of_parallelizable_code_writes_FASTER/100);
            }
            else if(strcmp(existing_system, "WT") == 0) // WiredTiger
            {
                overall_prop_parallelizable_code = ((double)(read_percentage + rmw_percentage + blind_update_percentage + long_scan_percentage + long_scan_empty_percentage)*prop_of_parallelizable_code_reads_WT/100) + ((double)write_percentage*prop_of_parallelizable_code_writes_WT/100);
            }
            else // Cosine
            {
                overall_prop_parallelizable_code = ((double)(read_percentage + rmw_percentage  + long_scan_percentage + long_scan_empty_percentage)*prop_of_parallelizable_code_reads_Cosine/100) + ((double)write_percentage*prop_of_parallelizable_code_writes_Cosine/100);
            }
        }
        printf("%f proportion of code is parallelizable.\n\n", overall_prop_parallelizable_code);
    }
    else
    {
        overall_prop_parallelizable_code = 1.0; 
    }
}


int main(int argc, char* argv[]) {

    int T, K, Z, Y;
	double M, M_B, M_BF, M_BC, M_FP, M_F;
	double data = 1000000000000;
    /**************************** ALL AWS DETAILS BEGIN ****************************/
    double MBps = 3500;
    B = 256*1024/(E);
    IOPS = MBps*pow(10,6)/(B*E);
    if(IOPS > 15000) 
    {
        IOPS = 15000;
    }
    /**************************** ALL AWS DETAILS END ****************************/

    /**************************** ALL WORKLOAD DETAILS BEGIN ****************************/
    s = 4096;
    query_count = 10000000000;
    read_percentage = 20; // point lookups
    write_percentage = 50; // inserts 
    rmw_percentage = 20; // read-modify writes 
    blind_update_percentage = 0; // blind updates
    short_scan_percentage = 0;
    long_scan_percentage = 0; // range queries
    long_scan_empty_percentage = 10; // empty range queries
    workload_type = atoi(argv[1]);
    /**************************** ALL WORKLOAD DETAILS END ****************************/

    double FPR_sum=1.0;

    if (workload_type == 0) 
    {
        printf("UNIFORM\n");
        //U = 1000000000;
        U = 100000000000000;
    }
    else
    {
        printf("SKEW\n");
        U_1 = 100000;
        U_2 = 100000000;
        p_put = 0.5;
        p_get = 0.5; //atof(argv[6]);
    }

    if(strcmp(argv[2], "WT") == 0)
    {
        // run with ./verify 0 WT 32 or ./verify 0 WT 64 
        T = 64; 
        K = 1;
        Z = 1;
        M = atof(argv[3])*(1024*1024*1024); //1968.0*(1024*1024*1024);
        M_B = M * (B*E) / ( (F*T) + (B*E) );
        M_BF = 0.0; 
        M_FP = M - M_B;
        //M_FP = M - M_B;
        M_F = M_BF + M_FP;
        getNoOfLevelsAvgCase(&L, M_B, T, data);
        Y = L;
        //L = 3; 
        printf("M_B:%f, M_F:%f L:%d\n", M_B/(1024*1024*1024), M_F/(1024*1024*1024), L);

        analyzeUpdateCostAvgCase(&update_cost, T, K, Z, L, Y, M, M_F, M_B, data, -1);
        analyzeReadCostAvgCase(&avg_read_cost, &FPR_sum, T, K, Z, L, Y, M, M_B, 0, M_F, M_BF, data, -1);
        avg_read_cost = avg_read_cost * B_TREE_CACHE_DISCOUNT_FACTOR; // Discounting factor due to caching in Btrees
        analyzeScanCostAvgCase(&long_scan_cost, T, K, Z, L, Y, M, M_B, M_F, M_BF, data);
        rmw_cost = avg_read_cost; 
        blind_update_cost = avg_read_cost;
        long_scan_empty_cost = 1.0; // 1 I/O to figure out that the query is empty
    }
    else if(strcmp(argv[2], "rocks") == 0)
    {
        // run with ./verify 0 rocks 32 or ./verify 0 rocks 64 
        T = 10; 
        K = 1;
        Z = 1;
        Y = 0;
        M = atof(argv[3])*(1024*1024*1024); 
        M_FP = (double)data*F/B;
        M_BF = data * 10.0/8.0; 
        //M_FP = 29.103830*(1024*1024*1024);
        M_F = M_BF + M_FP;
        M_B = M - M_F; //787.200000*(1024*1024*1024); //atof(argv[3])*(1024*1024*1024); 
        getNoOfLevelsAvgCase(&L, M_B, T, data);
        //L = 3; 
        printf("M_B:%f, M_F:%f L:%d\n", M_B/(1024*1024*1024), M_F/(1024*1024*1024), L);
        if(M_B < 0)
        {
            printf("Atleast %fGB memory needed\n", M_F/(1024*1024*1024));
        }

        analyzeUpdateCostAvgCase(&update_cost, T, K, Z, L, Y, M, M_F, M_B, data, -1);
        analyzeReadCostAvgCase(&avg_read_cost, &FPR_sum, T, K, Z, L, Y, M, M_B, 0, M_F, M_BF, data, -1);
        analyzeScanCostAvgCase(&long_scan_cost, T, K, Z, L, Y, M, M_B, M_F, M_BF, data);
        rmw_cost = update_cost + avg_read_cost; // Only done for LSM in this file
        blind_update_cost = update_cost;
        long_scan_empty_cost = L; // L I/Os to figure out that the query is empty
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

        analyzeUpdateCostAvgCase(&update_cost, T, K, Z, L, Y, M, M_F, M_B, data, -1);
        analyzeReadCostAvgCase(&avg_read_cost, &FPR_sum, T, K, Z, L, Y, M, M_B, 0, M_F, M_BF, data, -1);
        analyzeScanCostAvgCase(&long_scan_cost, T, K, Z, L, Y, M, M_B, M_F, M_BF, data);
        rmw_cost = avg_read_cost; 
        blind_update_cost = avg_read_cost;
        long_scan_empty_cost = 1.0; // 1 I/O to figure out that the query is empty  
    }
    else if(strcmp(argv[2], "ANY") == 0)
    {
        // run with ./verify 0 ANY 32 or ./verify 0 ANY 33333333333 100000000000000 64 5 4 1 1 6.4 16.6 9.0 4
        printf("ANY\n");
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
            // run with ./verify 0 ANY
            U = 100000000000000;
            B = 256*1024/(E);

            T = 16; 
            K = 15;
            Z = 4;
            Y = 0;
            M_B = 393.6*(1024*1024*1024); 
            M_BF = 1545.296170*(1024*1024*1024);
            M_FP = 29.103830*(1024*1024*1024);
            M_F = M_FP + M_BF;
            getNoOfLevelsAvgCase(&L, M_B, T, data);
            //L = 3;
            printf("M_B:%f, M_F:%f L:%d\n", M_B/(1024*1024*1024), M_F/(1024*1024*1024), L);
        }
        M = M_B + M_BF + M_FP; 
        M_F = M - M_B;

        analyzeUpdateCostAvgCase(&update_cost, T, K, Z, L, Y, M, M_F, M_B, data, -1);
        analyzeReadCostAvgCase(&avg_read_cost, &FPR_sum, T, K, Z, L, Y, M, M_B, 0, M_F, M_BF, data, -1);
        analyzeScanCostAvgCase(&long_scan_cost, T, K, Z, L, Y, M, M_B, M_F, M_BF, data);

        if(Z <= 0) // LSH Design space
        {
            rmw_cost =  avg_read_cost; 
            blind_update_cost = avg_read_cost;
        }
        else if (T%16==0 && K==1 && Z==1) // B-Tree Design space 
        {
            rmw_cost =  avg_read_cost; 
            blind_update_cost = avg_read_cost;
        }
        else // LSM Design space
        {
            rmw_cost = update_cost + avg_read_cost; 
            blind_update_cost = update_cost + avg_read_cost;
        }
    }
	if(FitsInMemory(M_B, data))
	{
		printf("%f entries (%f GB) fits within %f GB.\n", data, (data*E)/(1024*1024*1024), M_B/(1024*1024*1024));
		exit(0);
	}
    
    
    printf("read_cost: %f\n", avg_read_cost);
    printf("update_cost: %f\n", update_cost);
    printf("rmw_cost: %f\n", rmw_cost);
    printf("blind_update_cost: %f\n", blind_update_cost);
    printf("range_cost: %f\n", long_scan_cost);
    printf("empty range_cost: %f\n", long_scan_empty_cost);

    setOverallProportionOfParallelizableCode(argv[2]);
    double speedup = 1.0 / (1.0 - (overall_prop_parallelizable_code * (1.0 - (1.0/2) ) ) );
    double total_IO = query_count*(read_percentage*avg_read_cost + write_percentage*update_cost + rmw_percentage*rmw_cost + blind_update_percentage*blind_update_cost + long_scan_percentage*long_scan_cost + long_scan_empty_percentage*long_scan_empty_cost)/100;
    double latency = (total_IO/IOPS)/(60*60*24);
    double latency_after_speedup = latency/speedup;
    printf("IO: %f latency: %f day\n", total_IO, latency);
    printf("Latency after speedup: %f day\n", latency_after_speedup);
    return 0;
    
}
