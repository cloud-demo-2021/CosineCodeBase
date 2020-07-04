#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include <string.h>

#include "include/design_template.h"
#include "include/environment_variable.h"
#include "include/design_space_navigation.h"
#include "include/parameter_computation.h"
#include "include/update_cost.h"
#include "include/read_cost.h"
#include "include/workload.h"
#include "include/short_scan_cost.h"
#include "include/long_scan_cost.h"
#include "include/VM_library.h"
#include "include/continuum.h"


double min(double a, double b)
{
	return a < b? a : b;
}

void navigateDesignSpace1(double M, double M_B, design** d_list, char* mode)
{
	int T, K, Z, L, Y=0;
	double X, read_cost=0.0, avg_read_cost=0.0, avg_cpu_cost=0.0,update_cost=0.0, avg_update_cost=0.0, no_result_cost=0.0, no_result_cpu=0.0, short_scan_cost=0.0, long_scan_cost=0.0;
	double M_F_HI;
	double M_F; // = ((B*E + (M - M_F)) > 0 ? B*E + (M - M_F) : (B*E)); // byte
	double M_F_LO; // = (double)(M_B*(F)*T)/((B)*(E));
	double M_BF, M_FP;
	double FPR_sum=1.0;
	if(strcmp(mode, "BTREE") == 0)
	{
		for(T=32;T<=128;T = T*2)
		{
			K = 1;
	    	Z = 1;
	    	M_BF = 0.0;
	    	M_FP = M - M_B;
	    	M_F = M - M_B;
	    	if(scenario == 'A')
			{
				getNoOfLevelsAvgCase(&L, M_B, T, N); //(&update_cost, T, K, Z, L, Y, M, M_F, M_B);
			}
			else
			{
				getNoOfLevels(&L, M_B, T, N); //(&update_cost, T, K, Z, L, Y, M, M_F, M_B);
			}
			Y = L;

			if(write_percentage != 0)
			{
				if(scenario == 'A')
				{
					analyzeUpdateCostAvgCase(&update_cost, T, K, Z, L, Y, M, M_F, M_B, N);
				}
				else
				{
					analyzeUpdateCost(&update_cost, T, K, Z, L, Y, M, M_F, M_B);
				}
				//logUpdateCost(d_list, T, K, 0, L, Y, M, M_B, M_F, M_F_HI, M_F_LO, update_cost, read_cost, "");
			}
			if(read_percentage != 0)
			{
				analyzeReadCostAvgCase(&avg_read_cost, &FPR_sum, T, K, Z, L, Y, M, M_B, 0, M_F, M_BF, N);
				//analyzeReadCostAvgCaseCPU(&avg_cpu_cost, T, K, Z, L, Y, M, M_B, 0, M_F, M_BF);
				analyzeReadCost(&read_cost, &FPR_sum, T, K, Z, L, Y, M, M_B, 0, M_F, M_BF, N);
				analyzeNoResultCost(&no_result_cost, &FPR_sum, T, K, Z, L, Y, M, M_B, 0, M_F, M_BF, N);	
				//analyzeNoResultCPU(&no_result_cpu, T, K, Z, L, Y, M, M_B, 0, M_F, M_BF);	
			}
			if(long_scan_percentage != 0)
			{
				if(scenario == 'A')
				{
					analyzeScanCostAvgCase(&long_scan_cost, T, K, Z, L, Y, M, M_B, M_F, M_BF);
				}
				else
				{
					analyzeLongScanCost(&long_scan_cost, T, K, Z, L, Y, M, M_B, M_F, M_BF);
				}
			}
			logTotalCost(d_list, T, K, Z, L, Y, M, M_B, M_F, M_F_HI, M_F_LO, M_FP, M_BF, FPR_sum, update_cost, read_cost, avg_read_cost, no_result_cost, short_scan_cost, long_scan_cost, avg_cpu_cost, no_result_cpu, "B tree");
		}
    	return;
	}
	for(T=2;T<=32;T++)
	//T=10;
	{
		for(K=1;K<=T-1;K++)
		//K=6;
		{
			for(Z=1;Z<=T-1;Z++)
			//Z=1;
			{
				getX(&X, T, K, Z);
				M_F_HI = (double)N*((X/8)/T + (double)(F)/(B));
				getM_F_LO(&M_F_LO, &M_B, M, T, N);	
				if(M_B + M_F_LO > M) {
					continue;;	
				}
				set_M_F(&M_F, &M_B, M, M_F_HI, M_F_LO);
				if(scenario == 'A')
				{
					getNoOfLevelsAvgCase(&L, M_B, T, N); //(&update_cost, T, K, Z, L, Y, M, M_F, M_B);
				}
				else
				{
					getNoOfLevels(&L, M_B, T, N); //(&update_cost, T, K, Z, L, Y, M, M_F, M_B);
				}
				getY(&Y, &M_FP, M_F, M_F_HI, M_F_LO, X, T, L, N);
				getM_BF(&M_BF, M_F, M_FP);
				if(write_percentage != 0)
				{
					if(scenario == 'A')
					{
						analyzeUpdateCostAvgCase(&update_cost, T, K, Z, L, Y, M, M_F, M_B, N);
					}
					else
					{
						analyzeUpdateCost(&update_cost, T, K, Z, L, Y, M, M_F, M_B);
					}
					//logUpdateCost(d_list, T, K, 0, L, Y, M, M_B, M_F, M_F_HI, M_F_LO, update_cost, read_cost, "");
				}
				if(read_percentage != 0)
				{
					analyzeReadCostAvgCase(&avg_read_cost, &FPR_sum, T, K, Z, L, Y, M, M_B, 0, M_F, M_BF, N);
					analyzeReadCostAvgCaseCPU(&avg_cpu_cost, T, K, Z, L, Y, M, M_B, 0, M_F, M_BF);
					analyzeReadCost(&read_cost, &FPR_sum, T, K, Z, L, Y, M, M_B, 0, M_F, M_BF, N);
					analyzeNoResultCost(&no_result_cost, &FPR_sum, T, K, Z, L, Y, M, M_B, 0, M_F, M_BF, N);	
					analyzeNoResultCPU(&no_result_cpu, T, K, Z, L, Y, M, M_B, 0, M_F, M_BF);	
				}
				if(short_scan_percentage != 0)
				{
					analyzeShortScanCost(&short_scan_cost, T, K, Z, L, Y, M, M_B, M_F, M_BF);
				}
				if(long_scan_percentage != 0)
				{
					if(scenario == 'A')
					{
						analyzeScanCostAvgCase(&long_scan_cost, T, K, Z, L, Y, M, M_B, M_F, M_BF);
					}
					else
					{
						analyzeLongScanCost(&long_scan_cost, T, K, Z, L, Y, M, M_B, M_F, M_BF);
					}
				}

				logTotalCost(d_list, T, K, Z, L, Y, M, M_B, M_F, M_F_HI, M_F_LO, M_FP, M_BF, FPR_sum, update_cost, read_cost, avg_read_cost, no_result_cost, short_scan_cost, long_scan_cost, avg_cpu_cost, no_result_cpu, "LSM");
			}
		}
	}
	return;
}


void navigateDesignSpaceForContinuumSingleMachine(double M, double data, double workload, continuum_design** bucket, int compression_id)
{
	int T, K, Z, L, Y=0;
	double M_B = 0.0, M_BC = 0.0, M_B_temp; 
	double M_B_percent = 20;
	double X, read_cost=0.0, avg_read_cost=0.0, update_cost=0.0, no_result_cost=0.0, short_scan_cost=0.0, long_scan_cost=0.0, total_IO=0.0, min_IO=100000000000000;
	double M_F_HI, M_F, M_F_LO, M_BF, M_FP;
	double FPR_sum=1.0;

	consider_partial_last_level = false;
	consider_partial_buffer = false;
	while(M_B_percent < 100)
	{
		M_B = M_B_percent*M/100;
		M_FP = M - M_B;
		// **********************************************************************
		// BTREE design space
		// **********************************************************************
		//M_BC = M_B/2.0;
	    M_B = M_B - M_BC;
		M_BF = 0.0;
		M_F = M_FP + M_BF;
		total_IO = 0.0;
		for(T=32;T<=128;T = T*2)
		{
			K = 1;
	    	Z = 1;
		    if(scenario == 'A')
			{
				getNoOfLevelsAvgCase(&L, M_B, T, data);
			}
			else
			{
				getNoOfLevels(&L, M_B, T, data);
			}
			Y = L;

			if(write_percentage != 0)
			{
				if(scenario == 'A')
				{
					analyzeUpdateCostAvgCase(&update_cost, T, K, Z, L, Y, M, M_F, M_B, data);
					//analyzeUpdateCost(&update_cost, T, K, Z, L, Y, M, M_F, M_B);
				}
				else
				{
					analyzeUpdateCost(&update_cost, T, K, Z, L, Y, M, M_F, M_B);
				}
				total_IO += workload*write_percentage*update_cost/100;
			}
			if(read_percentage != 0)
			{
				if(scenario == 'A') //average case
				{
					analyzeReadCostAvgCase(&avg_read_cost, &FPR_sum, T, K, Z, L, Y, M, M_B, M_BC, M_F, M_BF, data);
					if(read_percentage > write_percentage)
					{
						avg_read_cost = avg_read_cost * 0.5;
					}
					total_IO += workload*read_percentage*avg_read_cost/100;
				}
				else //worst case
				{
					analyzeReadCost(&read_cost, &FPR_sum, T, K, Z, L, Y, M, M_B, 0.0, M_F, M_BF, data);
					total_IO += workload*read_percentage*read_cost/100;
				}
			}
			if(total_IO < min_IO)
			{
				min_IO = total_IO;
				(*bucket)->T = T;
				(*bucket)->K = K;
				(*bucket)->Z = Z;
				(*bucket)->L = L;
				(*bucket)->Y = Y;
				(*bucket)->M = M;
				(*bucket)->M_B = M_B;
				(*bucket)->M_BC = M_BC;
				(*bucket)->M_F = M_F;
				(*bucket)->M_FP = M_FP;
				(*bucket)->M_BF = M_BF;
				(*bucket)->FPR_sum = FPR_sum;
				(*bucket)->compression_id = compression_id;
				(*bucket)->update_cost = update_cost;
				(*bucket)->msg = "B-Tree";
				if(scenario == 'A')
				{
					(*bucket)->read_cost = avg_read_cost;
				}
				else
				{
					(*bucket)->read_cost = read_cost;
				}
				(*bucket)->short_scan_cost = short_scan_cost;
				(*bucket)->long_scan_cost = long_scan_cost;
				(*bucket)->total_cost = total_IO;
			}
		}
		M_B_percent = M_B_percent + 0.5;
	}
	

	// **********************************************************************
	// LSM design space
	// **********************************************************************
	M_B = 0.0;
	M_B_percent = 20;
	read_cost=0.0, avg_read_cost=0.0, update_cost=0.0, no_result_cost=0.0, short_scan_cost=0.0, long_scan_cost=0.0, total_IO=0.0;
	while(M_B < M)
	{
		M_B = M_B_percent*M/100;
		for(T=2;T<=15;T++)
		{
			for(K=1;K<=T-1;K++)
			{
				for(Z=1;Z<=T-1;Z++)
				{
					total_IO=0.0;
					getX(&X, T, K, Z);
					M_F_HI = data*((X/8)/T + (double)(F)/(B));
					getM_F_LO(&M_F_LO, &M_B, M, T, data);	
					if(M_B + M_F_LO > M) {
						continue;;	
					}
					set_M_F(&M_F, &M_B, M, M_F_HI, M_F_LO);
					if(scenario == 'A')
					{
						getNoOfLevelsAvgCase(&L, M_B, T, data);
					}
					else
					{
						getNoOfLevels(&L, M_B, T, data);
					}
					getY(&Y, &M_FP, M_F, M_F_HI, M_F_LO, X, T, L, data);
					getM_BF(&M_BF, M_F, M_FP);
					if(write_percentage != 0)
					{
						analyzeUpdateCostAvgCase(&update_cost, T, K, Z, L, Y, M, M_F, M_B, data);
						total_IO += workload*write_percentage*update_cost/100;
					}
					if(read_percentage != 0)
					{
						if(scenario == 'A') //average case
						{
							analyzeReadCostAvgCase(&avg_read_cost, &FPR_sum, T, K, Z, L, Y, M, M_B, 0, M_F, M_BF, data);
							//analyzeReadCostAvgCaseCPU(&avg_cpu_cost, T, K, Z, L, Y, M, M_B, M_F, M_BF);
							//analyzeNoResultCost(&no_result_cost, &FPR_sum, T, K, Z, L, Y, M, M_B, M_F, M_BF, data);	
							//analyzeNoResultCPU(&no_result_cpu, T, K, Z, L, Y, M, M_B, M_F, M_BF);	
							total_IO += workload*read_percentage*avg_read_cost/100;
						}
						else //worst case
						{
							analyzeReadCost(&read_cost, &FPR_sum, T, K, Z, L, Y, M, M_B, 0, M_F, M_BF, data);
							total_IO += workload*read_percentage*read_cost/100;
						}
					}
					if(short_scan_percentage != 0)
					{
						analyzeShortScanCost(&short_scan_cost, T, K, Z, L, Y, M, M_B, M_F, M_BF);
						total_IO += workload*short_scan_percentage*short_scan_cost/100;
					}
					if(long_scan_percentage != 0)
					{
						analyzeLongScanCost(&long_scan_cost, T, K, Z, L, Y, M, M_B, M_F, M_BF);
						total_IO += workload*long_scan_percentage*long_scan_cost/100;
					}
					if(total_IO < min_IO)
					{
						min_IO = total_IO;
						(*bucket)->T = T;
						(*bucket)->K = K;
						(*bucket)->Z = Z;
						(*bucket)->L = L;
						(*bucket)->Y = Y;
						(*bucket)->M = M;
						(*bucket)->M_B = M_B;
						(*bucket)->M_F = M_F;
						(*bucket)->M_FP = M_FP;
						(*bucket)->M_BF = M_BF;
						(*bucket)->FPR_sum = FPR_sum;
						(*bucket)->compression_id = compression_id;
						(*bucket)->update_cost = update_cost;
						(*bucket)->msg = "LSM";
						if(scenario == 'A')
						{
							(*bucket)->read_cost = avg_read_cost;
						}
						else
						{
							(*bucket)->read_cost = read_cost;
						}
						(*bucket)->short_scan_cost = short_scan_cost;
						(*bucket)->long_scan_cost = long_scan_cost;
						(*bucket)->total_cost = total_IO;
					}
					if((*bucket)->total_cost == 0)
					{
						exit(0);
					}
				}
			}
		}
		M_B_percent = M_B_percent + 20;
	}


	// **********************************************************************
	// LSH design space
	// **********************************************************************
	M_B = 0.0;
	M_B_percent = 20;
	read_cost=0.0, avg_read_cost=0.0, update_cost=0.0, no_result_cost=0.0, short_scan_cost=0.0, long_scan_cost=0.0, total_IO=0.0;
	int scale_factor = 8; 
	for(T=2;T<=32;T++)
	{
		K = T-1;
		for(Z=-1;Z<=0;Z++) // Z=-1 for hybrid logs, Z = 0 for append-only logs
		{
			total_IO=0.0;
			M_F = ((double)data/scale_factor)*(F)*(1.0 + (1.0/B)); 
			if(M_F > M)
			{
				continue;
			}
			M_B = M - M_F;
			L = 1;
			Y = 0;
			M_BF = 0.0;
			if(write_percentage != 0)
			{
				analyzeUpdateCostAvgCase(&update_cost, T, K, Z, L, Y, M, M_F, M_B, data);
				total_IO += workload*write_percentage*update_cost/100;
			}
			if(read_percentage != 0)
			{
				if(scenario == 'A') //average case
				{
					analyzeReadCostAvgCase(&avg_read_cost, &FPR_sum, T, K, Z, L, Y, M, M_B, 0, M_F, M_BF, data);
					total_IO += workload*read_percentage*avg_read_cost/100;
				}
				else //worst case
				{
					analyzeReadCost(&read_cost, &FPR_sum, T, K, Z, L, Y, M, M_B, 0, M_F, M_BF, data);
					total_IO += workload*read_percentage*read_cost/100;
				}
			}
			if(short_scan_percentage != 0)
			{
				analyzeShortScanCost(&short_scan_cost, T, K, Z, L, Y, M, M_B, M_F, M_BF);
				total_IO += workload*short_scan_percentage*short_scan_cost/100;
			}
			if(long_scan_percentage != 0)
			{
				analyzeLongScanCost(&long_scan_cost, T, K, Z, L, Y, M, M_B, M_F, M_BF);
				total_IO += workload*long_scan_percentage*long_scan_cost/100;
			}
			if(total_IO < min_IO)
			{
				min_IO = total_IO;
				(*bucket)->T = T;
				(*bucket)->K = K;
				(*bucket)->Z = Z;
				(*bucket)->L = L;
				(*bucket)->Y = Y;
				(*bucket)->M = M;
				(*bucket)->M_B = M_B;
				(*bucket)->M_F = M_F;
				(*bucket)->M_FP = M_FP;
				(*bucket)->M_BF = M_BF;
				(*bucket)->FPR_sum = FPR_sum;
				(*bucket)->compression_id = compression_id;
				(*bucket)->update_cost = update_cost;
				(*bucket)->msg = "LSH";
				if(scenario == 'A')
				{
					(*bucket)->read_cost = avg_read_cost;
				}
				else
				{
					(*bucket)->read_cost = read_cost;
				}
				(*bucket)->short_scan_cost = short_scan_cost;
				(*bucket)->long_scan_cost = long_scan_cost;
				(*bucket)->total_cost = total_IO;
			}
			if((*bucket)->total_cost == 0)
			{
				(*bucket)->total_cost = 0.1;
				//exit(0);
			}
		}
	}
}

int getCostForExistingSystems(double M, double data, double workload, continuum_design** bucket, double cost, int provider_id, int compression_id)
{
	int T, K, Z, L, Y=0;
	double M_B, M_BF, M_FP, M_F, M_BC = 0.0;
	double read_cost=0.0, avg_read_cost=0.0, update_cost=0.0, total_IO=0.0;
	double FPR_sum=1.0;
	if(strcmp(existing_system, "WT") == 0)
	{
		T = 64; 
		K = 1; 
		Z = 1; 
		M_B = M * (B*E) / ( (F*T) + (B*E) );
		M_FP = M - M_B;
	   	M_BF = 0.0; // 10 bits/entry in RocksDB is default and convert to byte because everything else is in byte
	    M_F = M_FP + M_BF;
	    if(M_F >= M)
	    {
	    	//printf("System %s needs at least %f GB of memory\n", existing_system, ((M_F/(1024*1024*1024))+1.0));
	    	return -1;
	    }
	    // WT uses 50% of memory to cache // Not using this M_BC anymore 
	    M_BC = 0.0;
	    //M_B = M_B - M_BC;
	   	//scenario == 'A'? getNoOfLevelsAvgCase(&L, M_B, T, data) : getNoOfLevels(&L, M_B, T, data);
	}
	if(strcmp(existing_system, "rocks") == 0)
	{
		T = 10; 
		K = 1; 
		Z = 1; 
		Y = 0; 
		M_FP = (double)N*F/B;
	   	M_BF = data * 10.0/8.0; // 10 bits/entry in RocksDB is default and convert to byte because everything else is in byte
	    M_F = M_FP + M_BF;
	    if(M_F >= M)
	    {
	    	//printf("System %s needs at least %f GB of memory\n", existing_system, ((M_F/(1024*1024*1024))+1.0));
	    	return -1;
	    }
	    M_B = (double)M - M_F;
	    M_B = M_B < 0? 0.0: M_B;
	    M_BC = 0.0;
	}
	if(strcmp(existing_system, "FASTER") == 0)
	{
		int scale_factor = 8; // We assume about N/scale_factor keys fit in the in-memory hash table 
		M_F = ((double)data/scale_factor)*(F)*(1.0 + (1.0/B)); // memory allocated for the hash index
		if(M_F >= M)
		{
			//printf("M: %f M_F: %f\n", M/(1024*1024*1024), M_F/(1024*1024*1024));
			return -1;	
		}
        M_B = M - M_F;
        T = (double)(data*E)/M_B; 
        K = T-1;
        Z = 0; // Asuming no cold levels
        M_BF = 0.0;
        Y = 0;
	    M_BC = 0.0;
	    L = 1;
	    //printf("T:%d, K:%d, Z:%d, M_B:%f, M_F:%f\n", T, K, Z, M_B/(1024*1024*1024), M_F/(1024*1024*1024));
	}
	if(strcmp(existing_system, "FASTER_H") == 0)
	{
		int scale_factor = 8; // We assume about N/scale_factor keys fit in the in-memory hash table 
		M_F = ((double)data/scale_factor)*(F)*(1.0 + (1.0/B)); // memory allocated for the hash index
		if(M_F >= M)
		{
			//printf("M: %f M_F: %f\n", M/(1024*1024*1024), M_F/(1024*1024*1024));
			return -1;	
		}
        M_B = M - M_F;
        T = (double)(data*E)/M_B; 
        K = T-1;
        Z = -1; // Asuming no cold levels
        M_BF = 0.0;
        Y = 0;
	    M_BC = 0.0;
	    L = 1;
	    //printf("T:%d, K:%d, Z:%d, M_B:%f, M_F:%f\n", T, K, Z, M_B/(1024*1024*1024), M_F/(1024*1024*1024));
	}
	if(strstr(existing_system, "FASTER") == NULL) // Any system other than FASTER
	//if(strcmp(existing_system, "FASTER") != 0) // Any system other than FASTER
	{
		scenario == 'A'? getNoOfLevelsAvgCase(&L, M_B, T, data) : getNoOfLevels(&L, M_B, T, data);
	}
	
	if(L <= 0)
	{
   		printf("L: %d \n", L);
	   	return -1;
	}
	if(strcmp(existing_system, "WT") == 0)
	{
		Y = L;
	}
	if(write_percentage != 0)
	{
		if(scenario == 'A') //average case
		{
			analyzeUpdateCostAvgCase(&update_cost, T, K, Z, L, Y, M, M_F, M_B, data);
			total_IO += workload*write_percentage*update_cost/100;
		}
		else
		{
			analyzeUpdateCost(&update_cost, T, K, Z, L, Y, M, M_F, M_B);
			total_IO += workload*write_percentage*update_cost/100;
		}
	}
	if(read_percentage != 0)
	{
		if(scenario == 'A') //average case
		{
			analyzeReadCostAvgCase(&avg_read_cost, &FPR_sum, T, K, Z, L, Y, M, M_B, M_BC, M_F, M_BF, data);
			// Discounting factor added for WT
			if(strcmp(existing_system, "WT") == 0 && read_percentage > write_percentage)
			{
				avg_read_cost = avg_read_cost * 0.5;
			}
			total_IO += workload*read_percentage*avg_read_cost/100;
		}
		else //worst case
		{
			analyzeReadCost(&read_cost, &FPR_sum, T, K, Z, L, Y, M, M_B, M_BC, M_F, M_BF, data);
			total_IO += workload*read_percentage*read_cost/100;
		}
	}
		
	(*bucket)->T = T;
	(*bucket)->K = K;
	(*bucket)->Z = Z;
	(*bucket)->L = L;
	(*bucket)->Y = T;
	(*bucket)->M = M;
	(*bucket)->M_B = M_B;
	(*bucket)->M_BC = M_BC;
	(*bucket)->M_F = M_F;
	(*bucket)->M_FP = M_FP;
	(*bucket)->M_BF = M_BF;
	(*bucket)->FPR_sum = FPR_sum;
	(*bucket)->update_cost = update_cost;
	if(scenario == 'A')
	{
		(*bucket)->read_cost = avg_read_cost;
	}
	else
	{
		(*bucket)->read_cost = read_cost;
	}
	(*bucket)->short_scan_cost = 0.0;
	(*bucket)->long_scan_cost = 0.0;
	(*bucket)->total_cost = total_IO;
	if(strstr(existing_system, "FASTER") != NULL && (*bucket)->total_cost == 0) // If system is any FASTER versions
	{
		(*bucket)->total_cost = 0.1;
	}
		
	//printf("Cost: %f Total IOs: %f, latency: %f hour\n", cost, total_IO, total_IO/(IOPS*3600.0));
 	return 0;
}

