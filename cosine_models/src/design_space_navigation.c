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
	double X, read_cost=0.0, avg_read_cost=0.0, rmw_cost=0.0, blind_update_cost=0.0, avg_cpu_cost=0.0,update_cost=0.0, avg_update_cost=0.0, no_result_cost=0.0, no_result_cpu=0.0, short_scan_cost=0.0, long_scan_cost=0.0;
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

			if(write_percentage != 0 || rmw_percentage != 0 || blind_update_percentage !=0)
			{
				if(scenario == 'A')
				{
					analyzeUpdateCostAvgCase(&update_cost, T, K, Z, L, Y, M, M_F, M_B, N, -1);
				}
				else
				{
					analyzeUpdateCost(&update_cost, T, K, Z, L, Y, M, M_F, M_B);
				}
				//logUpdateCost(d_list, T, K, 0, L, Y, M, M_B, M_F, M_F_HI, M_F_LO, update_cost, read_cost, "");
			}
			if(read_percentage != 0 || rmw_percentage != 0 || blind_update_percentage != 0)
			{
				analyzeReadCostAvgCase(&avg_read_cost, &FPR_sum, T, K, Z, L, Y, M, M_B, 0, M_F, M_BF, N, -1);
				//analyzeReadCostAvgCaseCPU(&avg_cpu_cost, T, K, Z, L, Y, M, M_B, 0, M_F, M_BF);
				analyzeReadCost(&read_cost, &FPR_sum, T, K, Z, L, Y, M, M_B, 0, M_F, M_BF, N);
				analyzeNoResultCost(&no_result_cost, &FPR_sum, T, K, Z, L, Y, M, M_B, 0, M_F, M_BF, N);	
				//analyzeNoResultCPU(&no_result_cpu, T, K, Z, L, Y, M, M_B, 0, M_F, M_BF);	
			}
			if(rmw_percentage != 0)
			{
				rmw_cost = scenario == 'A'? avg_read_cost: read_cost;
				rmw_cost += 1.0/B;
			}
			if(blind_update_percentage != 0)
			{
				blind_update_cost = scenario == 'A'? avg_read_cost: read_cost;
				blind_update_cost += 1.0/B;
			}
			if(long_scan_percentage != 0)
			{
				if(scenario == 'A')
				{
					analyzeScanCostAvgCase(&long_scan_cost, T, K, Z, L, Y, M, M_B, M_F, M_BF, N);
				}
				else
				{
					analyzeLongScanCost(&long_scan_cost, T, K, Z, L, Y, M, M_B, M_F, M_BF);
				}
			}
			logTotalCost(d_list, T, K, Z, L, Y, M, M_B, M_F, M_F_HI, M_F_LO, M_FP, M_BF, FPR_sum, update_cost, read_cost, avg_read_cost, rmw_cost, blind_update_cost, no_result_cost, short_scan_cost, long_scan_cost, avg_cpu_cost, no_result_cpu, "B tree");
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
				if(write_percentage != 0 || rmw_percentage != 0 || blind_update_percentage != 0)
				{
					if(scenario == 'A')
					{
						analyzeUpdateCostAvgCase(&update_cost, T, K, Z, L, Y, M, M_F, M_B, N, -1);
					}
					else
					{
						analyzeUpdateCost(&update_cost, T, K, Z, L, Y, M, M_F, M_B);
					}
					//logUpdateCost(d_list, T, K, 0, L, Y, M, M_B, M_F, M_F_HI, M_F_LO, update_cost, read_cost, "");
				}
				if(read_percentage != 0 || rmw_percentage != 0)
				{
					analyzeReadCostAvgCase(&avg_read_cost, &FPR_sum, T, K, Z, L, Y, M, M_B, 0, M_F, M_BF, N, -1);
					analyzeReadCostAvgCaseCPU(&avg_cpu_cost, T, K, Z, L, Y, M, M_B, 0, M_F, M_BF, -1);
					analyzeReadCost(&read_cost, &FPR_sum, T, K, Z, L, Y, M, M_B, 0, M_F, M_BF, N);
					analyzeNoResultCost(&no_result_cost, &FPR_sum, T, K, Z, L, Y, M, M_B, 0, M_F, M_BF, N);	
					analyzeNoResultCPU(&no_result_cpu, T, K, Z, L, Y, M, M_B, 0, M_F, M_BF);	
				}
				if(rmw_percentage != 0)
				{
					rmw_cost = scenario == 'A'?  avg_read_cost: read_cost;
					rmw_cost += update_cost;
				}
				if(blind_update_percentage != 0)
				{
					blind_update_cost = update_cost;
				}
				if(short_scan_percentage != 0)
				{
					analyzeShortScanCost(&short_scan_cost, T, K, Z, L, Y, M, M_B, M_F, M_BF);
				}
				if(long_scan_percentage != 0)
				{
					if(scenario == 'A')
					{
						analyzeScanCostAvgCase(&long_scan_cost, T, K, Z, L, Y, M, M_B, M_F, M_BF, N);
					}
					else
					{
						analyzeLongScanCost(&long_scan_cost, T, K, Z, L, Y, M, M_B, M_F, M_BF);
					}
				}

				logTotalCost(d_list, T, K, Z, L, Y, M, M_B, M_F, M_F_HI, M_F_LO, M_FP, M_BF, FPR_sum, update_cost, read_cost, avg_read_cost, rmw_cost, blind_update_cost, no_result_cost, short_scan_cost, long_scan_cost, avg_cpu_cost, no_result_cpu, "LSM");
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
	double X, read_cost=0.0, avg_read_cost=0.0, update_cost=0.0, rmw_cost=0.0, blind_update_cost=0.0, no_result_cost=0.0, short_scan_cost=0.0, long_scan_cost=0.0, long_scan_empty_cost=0.0, total_IO=0.0, min_IO=100000000000000;
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
		for(T=32;T<=128;T = T*2)
		{
			total_IO = 0.0;
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

			if(write_percentage != 0 || rmw_percentage  != 0 || blind_update_percentage != 0)
			{
				if(scenario == 'A')
				{
					analyzeUpdateCostAvgCase(&update_cost, T, K, Z, L, Y, M, M_F, M_B, data, -1);
					//analyzeUpdateCost(&update_cost, T, K, Z, L, Y, M, M_F, M_B);
				}
				else
				{
					analyzeUpdateCost(&update_cost, T, K, Z, L, Y, M, M_F, M_B);
				}
				total_IO += workload*write_percentage*update_cost/100;
			}
			if(read_percentage != 0 || rmw_percentage  != 0 || blind_update_percentage != 0)
			{
				if(scenario == 'A') //average case
				{
					analyzeReadCostAvgCase(&avg_read_cost, &FPR_sum, T, K, Z, L, Y, M, M_B, M_BC, M_F, M_BF, data, -1);
					avg_read_cost = avg_read_cost * B_TREE_CACHE_DISCOUNT_FACTOR; // Discounting factor due to caching in Btrees
					total_IO += workload*read_percentage*avg_read_cost/100;
				}
				else //worst case
				{
					analyzeReadCost(&read_cost, &FPR_sum, T, K, Z, L, Y, M, M_B, 0.0, M_F, M_BF, data);
					total_IO += workload*read_percentage*read_cost/100;
				}
			}

			if(rmw_percentage != 0)
			{
				rmw_cost = scenario == 'A'? avg_read_cost : read_cost;
				rmw_cost += (1.0/B);
				total_IO += workload*rmw_percentage*rmw_cost/100;
			}

			if(blind_update_percentage != 0)
			{
				blind_update_cost = scenario == 'A'? avg_read_cost : read_cost;
				blind_update_cost += (1.0/B);
				total_IO += workload*blind_update_percentage*blind_update_cost/100;
			}

			if(long_scan_percentage != 0)
			{
				if(scenario == 'A') //average case
				{
					analyzeScanCostAvgCase(&long_scan_cost, T, K, Z, L, Y, M, M_B, M_F, M_BF, data);
				}
				else //worst case
				{
					analyzeLongScanCost(&long_scan_cost, T, K, Z, L, Y, M, M_B, M_F, M_BF);
				}
				total_IO += workload*long_scan_percentage*long_scan_cost/100;
			}
			if(long_scan_empty_percentage != 0)
			{
				if(scenario == 'A') //average case
				{
					long_scan_empty_cost = 1.0; // 1 I/O to figure out that the query is empty
				}
				else //worst case
				{
					analyzeLongScanCost(&long_scan_empty_cost, T, K, Z, L, Y, M, M_B, M_F, M_BF);
				}
				total_IO += workload*long_scan_empty_percentage*long_scan_empty_cost/100;
				
			}

			if(total_IO < min_IO)
			{
				min_IO = total_IO;
				(*bucket)->T = T;
				(*bucket)->K = K;
				(*bucket)->Z = Z;
				(*bucket)->L = L;
				(*bucket)->Y = Y;
				(*bucket)->C = -1;
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
				(*bucket)->rmw_cost = rmw_cost;
				(*bucket)->blind_update_cost = blind_update_cost;
				(*bucket)->short_scan_cost = short_scan_cost;
				(*bucket)->long_scan_cost = long_scan_cost;
				(*bucket)->long_scan_empty_cost = long_scan_empty_cost;
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
	bool break_from_C_loop = false;
	while(M_B < M)
	{
		M_B = M_B_percent*M/100;
		for(T=2;T<=15;T++)
		{
			for(K=1;K<=T-1;K++)
			{
				for(Z=1;Z<=T-1;Z++)
				{
					break_from_C_loop = false;
					for(int C=1;C<=T-1 && break_from_C_loop==false;C++)
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

						/* check if CLL applies */
						if(Y == 0 && enable_CLL && C != -1) // CLL can be applied because the basic mem specs doesn't lead to cold levels
						{
							getNoOfLevelsWacky(&L, M_B, T, data, C);
						}	
						else
						{
							break_from_C_loop = true; // No need to continue in this loop for the next value of C. We may proceed to the next Z.
						}
						

						getM_BF(&M_BF, M_F, M_FP);
						if(write_percentage != 0 || rmw_percentage != 0 || blind_update_percentage != 0)
						{
							if(scenario == 'A') //average case
							{
								analyzeUpdateCostAvgCase(&update_cost, T, K, Z, L, Y, M, M_F, M_B, data, C);
							}
							else //worst case
							{
								analyzeUpdateCost(&update_cost, T, K, Z, L, Y, M, M_F, M_B);
							}
							total_IO += workload*write_percentage*update_cost/100;
						}
						if(read_percentage != 0 || rmw_percentage != 0 || blind_update_percentage != 0)
						{
							if(scenario == 'A') //average case
							{
								analyzeReadCostAvgCase(&avg_read_cost, &FPR_sum, T, K, Z, L, Y, M, M_B, 0, M_F, M_BF, data, C);
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
						if(rmw_percentage != 0)
						{
							double rmw_cost_get = 0.0, rmw_cost_put = 0.0;
							if(scenario == 'A') //average case
							{
								analyzeReadCostAvgCase(&rmw_cost_get, &FPR_sum, T, K, Z, L, Y, M, M_B, 0, M_F, M_BF, data, C);
								analyzeUpdateCostAvgCase(&rmw_cost_put, T, K, Z, L, Y, M, M_F, M_B, data, C);
							}
							else //worst case
							{
								analyzeReadCost(&rmw_cost_get, &FPR_sum, T, K, Z, L, Y, M, M_B, 0, M_F, M_BF, data);
								analyzeUpdateCost(&rmw_cost_put, T, K, Z, L, Y, M, M_F, M_B);
							}
							rmw_cost = rmw_cost_get + rmw_cost_put;
							total_IO += workload*rmw_percentage*rmw_cost/100;
						}
						if(blind_update_percentage != 0)
						{
							if(scenario == 'A') //average case
							{
								analyzeUpdateCostAvgCase(&blind_update_cost, T, K, Z, L, Y, M, M_F, M_B, data, C);
							}
							else //worst case
							{
								analyzeUpdateCost(&blind_update_cost, T, K, Z, L, Y, M, M_F, M_B);
							}
							total_IO += workload*blind_update_percentage*blind_update_cost/100;
						}
						if(short_scan_percentage != 0)
						{
							analyzeShortScanCost(&short_scan_cost, T, K, Z, L, Y, M, M_B, M_F, M_BF);
							total_IO += workload*short_scan_percentage*short_scan_cost/100;
						}
						if(long_scan_percentage != 0)
						{
							if(scenario == 'A') //average case
							{
								analyzeScanCostAvgCase(&long_scan_cost, T, K, Z, L, Y, M, M_B, M_F, M_BF, data);
							}
							else //worst case
							{
								analyzeLongScanCost(&long_scan_cost, T, K, Z, L, Y, M, M_B, M_F, M_BF);
							}
							total_IO += workload*long_scan_percentage*long_scan_cost/100;
						}
						if(long_scan_empty_percentage != 0)
						{
							if(scenario == 'A') //average case
							{
								analyzeEmptyScanCost(&long_scan_empty_cost, T, K, Z, L, Y, M, M_B, M_F, M_BF, data);
							}
							else //worst case
							{
								analyzeLongScanCost(&long_scan_empty_cost, T, K, Z, L, Y, M, M_B, M_F, M_BF);
							}
							total_IO += workload*long_scan_empty_percentage*long_scan_empty_cost/100;	
						}
						if(total_IO < min_IO)
						{
							min_IO = total_IO;
							(*bucket)->T = T;
							(*bucket)->K = K;
							(*bucket)->Z = Z;
							(*bucket)->L = L;
							(*bucket)->Y = Y;
							(*bucket)->C = break_from_C_loop == true? -1 : C;
							(*bucket)->M = M;
							(*bucket)->M_B = M_B;
							(*bucket)->M_F = M_F;
							(*bucket)->M_FP = M_FP;
							(*bucket)->M_BF = M_BF;
							(*bucket)->FPR_sum = FPR_sum;
							(*bucket)->compression_id = compression_id;
							(*bucket)->update_cost = update_cost;
							(*bucket)->rmw_cost = rmw_cost;
							(*bucket)->blind_update_cost = blind_update_cost;
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
							(*bucket)->long_scan_empty_cost = long_scan_empty_cost;
							(*bucket)->total_cost = total_IO;
						}
						if((*bucket)->total_cost == 0)
						{
							exit(0);
						}
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
	M_F = ((double)data/scale_factor)*(F)*(1.0 + (1.0/B)); 
	//for(T=2;T<=200;T++)
	{
		for(Z=-1;Z<=0;Z++) // Z=-1 for hybrid logs, Z = 0 for append-only logs
		{
			total_IO=0.0;
			if(M_F > M)
			{
				continue;
			}
			M_B = M - M_F;
			T = (data*E)/M_B;
			K = T-1;
			L = 1;
			Y = 0;
			M_BF = 0.0;
			if(write_percentage != 0 || rmw_percentage != 0 || blind_update_percentage != 0)
			{
				if(scenario == 'A') //average case
				{
					analyzeUpdateCostAvgCase(&update_cost, T, K, Z, L, Y, M, M_F, M_B, data, -1);
				}
				else
				{
					analyzeUpdateCost(&update_cost, T, K, Z, L, Y, M, M_F, M_B);
				}
				total_IO += workload*write_percentage*update_cost/100;
			}
			if(read_percentage != 0 || rmw_percentage != 0 || blind_update_percentage != 0)
			{
				if(scenario == 'A') //average case
				{
					analyzeReadCostAvgCase(&avg_read_cost, &FPR_sum, T, K, Z, L, Y, M, M_B, 0, M_F, M_BF, data, -1);
					total_IO += workload*read_percentage*avg_read_cost/100;
				}
				else //worst case
				{
					analyzeReadCost(&read_cost, &FPR_sum, T, K, Z, L, Y, M, M_B, 0, M_F, M_BF, data);
					total_IO += workload*read_percentage*read_cost/100;
				}
			}
			if(rmw_percentage != 0)
			{
				rmw_cost = scenario == 'A'? avg_read_cost : read_cost;
				rmw_cost += (1.0/B);
				total_IO += workload*rmw_percentage*rmw_cost/100;
			}
			if(blind_update_percentage != 0)
			{
				blind_update_cost = scenario == 'A'? avg_read_cost : read_cost;
				blind_update_cost += (1.0/B);
				total_IO += workload*blind_update_percentage*blind_update_cost/100;
			}
			if(short_scan_percentage != 0)
			{
				analyzeShortScanCost(&short_scan_cost, T, K, Z, L, Y, M, M_B, M_F, M_BF);
				total_IO += workload*short_scan_percentage*short_scan_cost/100;
			}
			if(long_scan_percentage != 0)
			{
				// Note: for LSH in Cosine, long scan cost is same for both W case and A case - becaue scans are not implemented in LSH
				analyzeScanCostAvgCase(&long_scan_cost, T, K, Z, L, Y, M, M_B, M_F, M_BF, data); // Worst-case and avg-case scans considered same for LSH-tables
				total_IO += workload*long_scan_percentage*long_scan_cost/100;
			}
			if(long_scan_empty_percentage != 0)
			{
				long_scan_empty_cost = 1.0; // 1 I/O to figure out that the query is empty
				total_IO += workload*long_scan_empty_percentage*long_scan_empty_cost/100;
			}
			if(total_IO < min_IO)
			{
				min_IO = total_IO;
				(*bucket)->T = T;
				(*bucket)->K = K;
				(*bucket)->Z = Z;
				(*bucket)->L = L;
				(*bucket)->Y = Y;
				(*bucket)->C = -1;
				(*bucket)->M = M;
				(*bucket)->M_B = M_B;
				(*bucket)->M_F = M_F;
				(*bucket)->M_FP = M_FP;
				(*bucket)->M_BF = M_BF;
				(*bucket)->FPR_sum = FPR_sum;
				(*bucket)->compression_id = compression_id;
				(*bucket)->update_cost = update_cost;
				(*bucket)->rmw_cost = rmw_cost;
				(*bucket)->blind_update_cost = blind_update_cost;
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
				(*bucket)->long_scan_empty_cost = long_scan_empty_cost;
				(*bucket)->total_cost = total_IO;
			}
			if((*bucket)->total_cost == 0)
			{
				(*bucket)->total_cost = 0.1;
				//exit(0);
			}
		}
	}

	// **********************************************************************
	// FASTER_H design (specifically testing if FASTER_H is better)
	// **********************************************************************
	read_cost=0.0, avg_read_cost=0.0, update_cost=0.0, no_result_cost=0.0, short_scan_cost=0.0, long_scan_cost=0.0, total_IO=0.0;
	scale_factor = 8; 
	M_F = ((double)data/scale_factor)*(F)*(1.0 + (1.0/B)); // memory allocated for the hash index
	if(M_F < M)
	{
		M_B = M - M_F;
		T = (double)(data*E)/M_B; 
		K = T-1;
		Z = -1; // Asuming no cold levels
		M_BF = 0.0;
		Y = 0;
		M_BC = 0.0;
		L = 1;

		if(write_percentage != 0 || rmw_percentage != 0 || blind_update_percentage != 0)
		{
			if(scenario == 'A') //average case
			{
				analyzeUpdateCostAvgCase(&update_cost, T, K, Z, L, Y, M, M_F, M_B, data, -1);
			}
			else
			{
				analyzeUpdateCost(&update_cost, T, K, Z, L, Y, M, M_F, M_B);
			}
			total_IO += workload*write_percentage*update_cost/100;
		}
		if(read_percentage != 0 || rmw_percentage != 0 || blind_update_percentage != 0)
		{
			if(scenario == 'A') //average case
			{
				analyzeReadCostAvgCase(&avg_read_cost, &FPR_sum, T, K, Z, L, Y, M, M_B, 0, M_F, M_BF, data, -1);
				total_IO += workload*read_percentage*avg_read_cost/100;
			}
			else //worst case
			{
				analyzeReadCost(&read_cost, &FPR_sum, T, K, Z, L, Y, M, M_B, 0, M_F, M_BF, data);
				total_IO += workload*read_percentage*read_cost/100;
			}
		}
		if(rmw_percentage != 0)
		{
			rmw_cost = scenario == 'A'? avg_read_cost : read_cost;
			rmw_cost += (1.0/B);
			total_IO += workload*rmw_percentage*rmw_cost/100;
		}
		if(blind_update_percentage != 0)
		{
			blind_update_cost = scenario == 'A'? avg_read_cost : read_cost;
			blind_update_cost += (1.0/B);
			total_IO += workload*blind_update_percentage*blind_update_cost/100;
		}
		if(short_scan_percentage != 0)
		{
			analyzeShortScanCost(&short_scan_cost, T, K, Z, L, Y, M, M_B, M_F, M_BF);
			total_IO += workload*short_scan_percentage*short_scan_cost/100;
		}
		if(long_scan_percentage != 0)
		{
			// Note: for LSH in Cosine, long scan cost is same for both W case and A case - becaue scans are not implemented in LSH
			analyzeScanCostAvgCase(&long_scan_cost, T, K, Z, L, Y, M, M_B, M_F, M_BF, data); // Worst-case and avg-case scans considered same for LSH-tables
			total_IO += workload*long_scan_percentage*long_scan_cost/100;
		}
		if(long_scan_empty_percentage != 0)
		{
			long_scan_empty_cost = 1.0; // 1 I/O to figure out that the query is empty
			total_IO += workload*long_scan_empty_percentage*long_scan_empty_cost/100;
		}
		if(total_IO < min_IO)
		{
			min_IO = total_IO;
			(*bucket)->T = T;
			(*bucket)->K = K;
			(*bucket)->Z = Z;
			(*bucket)->L = L;
			(*bucket)->Y = Y;
			(*bucket)->C = -1;
			(*bucket)->M = M;
			(*bucket)->M_B = M_B;
			(*bucket)->M_F = M_F;
			(*bucket)->M_FP = M_FP;
			(*bucket)->M_BF = M_BF;
			(*bucket)->FPR_sum = FPR_sum;
			(*bucket)->compression_id = compression_id;
			(*bucket)->update_cost = update_cost;
			(*bucket)->rmw_cost = rmw_cost;
			(*bucket)->blind_update_cost = blind_update_cost;
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
			(*bucket)->long_scan_empty_cost = long_scan_empty_cost;
			(*bucket)->total_cost = total_IO;
		}
		if((*bucket)->total_cost == 0)
		{
			(*bucket)->total_cost = 0.1;
			//exit(0);
		}
	}
}

int getCostForExistingSystemsDefaultDesign(double M, double data, double workload, continuum_design** bucket, double cost, int provider_id, int compression_id)
{
	int T, K, Z, L, Y=0;
	double M_B, M_BF, M_FP, M_F, M_BC = 0.0;
	double read_cost=0.0, avg_read_cost=0.0, rmw_cost=0.0, blind_update_cost=0.0, update_cost=0.0, long_scan_cost=0.0, long_scan_empty_cost=0.0, total_IO=0.0;
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
		M_FP = (double)data*F/B;
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
	/* Get write cost */
	{
		if(scenario == 'A') //average case
		{
			analyzeUpdateCostAvgCase(&update_cost, T, K, Z, L, Y, M, M_F, M_B, data, -1);
			total_IO += workload*write_percentage*update_cost/100;
		}
		else
		{
			analyzeUpdateCost(&update_cost, T, K, Z, L, Y, M, M_F, M_B);
			total_IO += workload*write_percentage*update_cost/100;
		}
	}
	
	/* Get read cost */
	{
		if(scenario == 'A') //average case
		{
			analyzeReadCostAvgCase(&avg_read_cost, &FPR_sum, T, K, Z, L, Y, M, M_B, M_BC, M_F, M_BF, data, -1);
			// Discounting factor added for WT
			if(strcmp(existing_system, "WT") == 0)
			{
				avg_read_cost = avg_read_cost * B_TREE_CACHE_DISCOUNT_FACTOR;
			}
			total_IO += workload*read_percentage*avg_read_cost/100;
		}
		else //worst case
		{
			analyzeReadCost(&read_cost, &FPR_sum, T, K, Z, L, Y, M, M_B, M_BC, M_F, M_BF, data);
			total_IO += workload*read_percentage*read_cost/100;
		}
	}

	/* Get rmw cost */
	{
		if(strcmp(existing_system, "rocks") == 0)
		{
			rmw_cost = scenario == 'A'? avg_read_cost: read_cost;
			rmw_cost += update_cost;
		}
		else
		{
			rmw_cost = scenario == 'A'? avg_read_cost: read_cost;
			rmw_cost += 1.0/B;
		}
		total_IO += workload*rmw_percentage*rmw_cost/100;
	}

	/* Get blind update cost */
	{
		if(strcmp(existing_system, "rocks") == 0)
		{
			blind_update_cost = update_cost;
		}
		else
		{
			blind_update_cost = scenario == 'A'? avg_read_cost: read_cost;
			blind_update_cost += 1.0/B;
		}
		total_IO += workload*blind_update_percentage*blind_update_cost/100;
	}

	/* Get range query cost */
	{
		if(scenario == 'A') //average case
		{
			analyzeScanCostAvgCase(&long_scan_cost, T, K, Z, L, Y, M, M_B, M_F, M_BF, data);
		}
		else
		{
			analyzeLongScanCost(&long_scan_cost, T, K, Z, L, Y, M, M_B, M_F, M_BF);
		}
		total_IO += workload*long_scan_percentage*long_scan_cost/100;
	}

	/* Get empty range query cost */
	if(strcmp(existing_system, "WT") == 0)
	{
		long_scan_empty_cost = 1.0; // 1 I/O to figure out that the query is empty
	}
	else if(strcmp(existing_system, "rocks") == 0)
	{
		analyzeEmptyScanCost(&long_scan_empty_cost, T, K, Z, L, Y, M, M_B, M_F, M_BF, data);
	}
	else // FASTER versions
	{ 
		long_scan_empty_cost = 1.0; // 1 I/O to figure out that the query is empty	
	}
	total_IO += workload*long_scan_empty_percentage*long_scan_empty_cost/100;

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
	(*bucket)->update_cost = update_cost;
	(*bucket)->rmw_cost = rmw_cost;
	(*bucket)->blind_update_cost = blind_update_cost;
	if(scenario == 'A')
	{
		(*bucket)->read_cost = avg_read_cost;
	}
	else
	{
		(*bucket)->read_cost = read_cost;
	}
	(*bucket)->short_scan_cost = 0.0;
	(*bucket)->long_scan_cost = long_scan_cost;
	(*bucket)->long_scan_empty_cost = long_scan_empty_cost;
	(*bucket)->total_cost = total_IO;
	if(strstr(existing_system, "FASTER") != NULL && (*bucket)->total_cost == 0) // If system is any FASTER versions
	{
		(*bucket)->total_cost = 0.1;
	}
		
	//printf("Cost: %f Total IOs: %f, latency: %f hour\n", cost, total_IO, total_IO/(IOPS*3600.0));
 	return 0;
}

int getCostForExistingSystemsTunedDesign(double M, double data, double workload, continuum_design** bucket, double cost, int provider_id, int compression_id)
{
	int T, K, Z, L, Y=0;
	double M_B, M_BF, M_FP, M_F, M_BC = 0.0;
	double read_cost=0.0, avg_read_cost=0.0, rmw_cost=0.0, blind_update_cost=0.0, update_cost=0.0, total_IO=0.0, min_IO=100000000000000;;
	double FPR_sum=1.0;

	// **********************************************************************
	// WT design space
	// **********************************************************************
	if(strcmp(existing_system, "WT") == 0)
	{
		//printf("Into WT\n");
		K = 1; 
		Z = 1;
		M_BF = 0.0; 
		// WT uses 50% of memory to cache // Not using this M_BC anymore 
		M_BC = 0.0;
		for(T = 32; T <= 256; T++)
		{ 
			M_B = M * (B*E) / ( (F*T) + (B*E) );
			M_FP = M - M_B;
		    M_F = M_FP + M_BF;
		    if(M_F >= M)
		    {
		    	return -1;
		    }
		    scenario == 'A'? getNoOfLevelsAvgCase(&L, M_B, T, data) : getNoOfLevels(&L, M_B, T, data);
		    if(L <= 0)
			{
		   		printf("L: %d \n", L);
			   	return -1;
			}
			Y = L;
			if(write_percentage != 0 || rmw_percentage != 0 || blind_update_percentage != 0)
			{
				if(scenario == 'A') //average case
				{
					analyzeUpdateCostAvgCase(&update_cost, T, K, Z, L, Y, M, M_F, M_B, data, -1);
					total_IO += workload*write_percentage*update_cost/100;
				}
				else
				{
					analyzeUpdateCost(&update_cost, T, K, Z, L, Y, M, M_F, M_B);
					total_IO += workload*write_percentage*update_cost/100;
				}
			}
			if(read_percentage != 0 || rmw_percentage != 0 || blind_update_percentage != 0)
			{
				if(scenario == 'A') //average case
				{
					analyzeReadCostAvgCase(&avg_read_cost, &FPR_sum, T, K, Z, L, Y, M, M_B, M_BC, M_F, M_BF, data, -1);
					// Discounting factor added for WT
					if(strcmp(existing_system, "WT") == 0)
					{
						avg_read_cost = avg_read_cost * B_TREE_CACHE_DISCOUNT_FACTOR;
					}
					total_IO += workload*read_percentage*avg_read_cost/100;
				}
				else //worst case
				{
					analyzeReadCost(&read_cost, &FPR_sum, T, K, Z, L, Y, M, M_B, M_BC, M_F, M_BF, data);
					total_IO += workload*read_percentage*read_cost/100;
				}
			}
			if(rmw_percentage != 0)
			{
				rmw_cost = scenario == 'A'? avg_read_cost: read_cost;
				rmw_cost += (1.0/B);
				total_IO += workload*rmw_percentage*rmw_cost/100;
			}
			if(blind_update_percentage != 0)
			{
				blind_update_cost = scenario == 'A'? avg_read_cost: read_cost;
				blind_update_cost += (1.0/B);
				total_IO += workload*blind_update_percentage*blind_update_cost/100;
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
				(*bucket)->rmw_cost = rmw_cost;
				(*bucket)->blind_update_cost = blind_update_cost;
				(*bucket)->msg = "B-Tree";
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
			}
		}
	}
	
	// **********************************************************************
	// RocksDB design space
	// **********************************************************************

	if(strcmp(existing_system, "rocks") == 0)
	{
		//printf("Into RocksDB\n");
		K = 1; 
		Z = 1; 
		Y = 0; 
		M_BC = 0.0;
		for(T=2;T<=15;T++)
		{
			M_FP = (double)data*F/B;
		   	M_BF = data * 10.0/8.0; // 10 bits/entry in RocksDB is default and convert to byte because everything else is in byte
		    M_F = M_FP + M_BF;
		    if(M_F >= M)
		    {
		    	return -1;
		    }
		    M_B = (double)M - M_F;
		    M_B = M_B < 0? 0.0: M_B;
		    scenario == 'A'? getNoOfLevelsAvgCase(&L, M_B, T, data) : getNoOfLevels(&L, M_B, T, data);
		    if(L <= 0)
			{
		   		printf("L: %d \n", L);
			   	return -1;
			}
			if(write_percentage != 0 || rmw_percentage != 0 || blind_update_percentage != 0)
			{
				if(scenario == 'A') //average case
				{
					analyzeUpdateCostAvgCase(&update_cost, T, K, Z, L, Y, M, M_F, M_B, data, -1);
					total_IO += workload*write_percentage*update_cost/100;
				}
				else
				{
					analyzeUpdateCost(&update_cost, T, K, Z, L, Y, M, M_F, M_B);
					total_IO += workload*write_percentage*update_cost/100;
				}
			}
			if(read_percentage != 0 || rmw_percentage != 0 || blind_update_percentage != 0) 
			{
				if(scenario == 'A') //average case
				{
					analyzeReadCostAvgCase(&avg_read_cost, &FPR_sum, T, K, Z, L, Y, M, M_B, M_BC, M_F, M_BF, data, -1);
					total_IO += workload*read_percentage*avg_read_cost/100;
				}
				else //worst case
				{
					analyzeReadCost(&read_cost, &FPR_sum, T, K, Z, L, Y, M, M_B, M_BC, M_F, M_BF, data);
					total_IO += workload*read_percentage*read_cost/100;
				}
			}
			if(rmw_percentage != 0)
			{
				rmw_cost = scenario == 'A'? avg_read_cost: read_cost;
				rmw_cost += update_cost;
				total_IO += workload*rmw_percentage*rmw_cost/100;
			}
			if(blind_update_percentage != 0)
			{
				blind_update_cost = update_cost;
				total_IO += workload*blind_update_percentage*blind_update_cost/100;
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
				(*bucket)->rmw_cost = rmw_cost;
				(*bucket)->blind_update_cost = blind_update_cost;
				(*bucket)->short_scan_cost = 0.0;
				(*bucket)->long_scan_cost = 0.0;
				(*bucket)->total_cost = total_IO;
			}
		}
	}

	// **********************************************************************
	// FASTER design space
	// **********************************************************************

	if(strcmp(existing_system, "FASTER") == 0)
	{
		Z = 0; // Asuming no cold levels
        M_BF = 0.0;
        Y = 0;
	    M_BC = 0.0;
	    L = 1;
		for(int scale_factor = 4; scale_factor <= 100; scale_factor+=4) // We assume about N/scale_factor keys fit in the in-memory hash table 
		{
			M_F = ((double)data/scale_factor)*(F)*(1.0 + (1.0/B)); // memory allocated for the hash index
			if(M_F >= M)
			{	
				continue;
				//return -1;	
			}
	        M_B = M - M_F;
	        T = (double)(data*E)/M_B; 
	        K = T-1;
	        if(write_percentage != 0 || rmw_percentage != 0 || blind_update_percentage != 0)
			{
				if(scenario == 'A') //average case
				{
					analyzeUpdateCostAvgCase(&update_cost, T, K, Z, L, Y, M, M_F, M_B, data, -1);
					total_IO += workload*write_percentage*update_cost/100;
				}
				else
				{
					analyzeUpdateCost(&update_cost, T, K, Z, L, Y, M, M_F, M_B);
					total_IO += workload*write_percentage*update_cost/100;
				}
			}
			if(read_percentage != 0 || rmw_percentage != 0 || blind_update_percentage != 0)
			{
				if(scenario == 'A') //average case
				{
					analyzeReadCostAvgCase(&avg_read_cost, &FPR_sum, T, K, Z, L, Y, M, M_B, M_BC, M_F, M_BF, data, -1);
					total_IO += workload*read_percentage*avg_read_cost/100;
				}
				else //worst case
				{
					analyzeReadCost(&read_cost, &FPR_sum, T, K, Z, L, Y, M, M_B, M_BC, M_F, M_BF, data);
					total_IO += workload*read_percentage*read_cost/100;
				}
			}
			if(rmw_percentage != 0)
			{
				rmw_cost = scenario == 'A'? avg_read_cost : read_cost;
				rmw_cost += (1.0/B);
				total_IO += workload*rmw_percentage*rmw_cost/100;
			}
			if(blind_update_percentage != 0)
			{
				blind_update_cost = scenario == 'A'? avg_read_cost : read_cost;
				blind_update_cost += (1.0/B);
				total_IO += workload*blind_update_percentage*blind_update_cost/100;
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
				(*bucket)->rmw_cost = rmw_cost;
				(*bucket)->blind_update_cost = blind_update_cost;
				(*bucket)->msg = "FASTER";
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
				if((*bucket)->total_cost == 0) // If system is any FASTER versions
				{
					(*bucket)->total_cost = 0.1;
				}
			}
    	}
	}

	// **********************************************************************
	// FASTER_H design space
	// **********************************************************************

	if(strcmp(existing_system, "FASTER_H") == 0)
	{
		Z = -1; // Asuming no cold levels
        M_BF = 0.0;
        Y = 0;
	    M_BC = 0.0;
	    L = 1;

		for(int scale_factor = 4; scale_factor <= 100; scale_factor+=4) // We assume about N/scale_factor keys fit in the in-memory hash table 
		{
			M_F = ((double)data/scale_factor)*(F)*(1.0 + (1.0/B)); // memory allocated for the hash index
			if(M_F >= M)
			{	
				continue;
				//return -1;	
			}
	        M_B = M - M_F;
	        T = (double)(data*E)/M_B; 
	        K = T-1;
	        if(write_percentage != 0 || rmw_percentage != 0 || blind_update_percentage != 0)
			{
				if(scenario == 'A') //average case
				{
					analyzeUpdateCostAvgCase(&update_cost, T, K, Z, L, Y, M, M_F, M_B, data, -1);
					total_IO += workload*write_percentage*update_cost/100;
				}
				else
				{
					analyzeUpdateCost(&update_cost, T, K, Z, L, Y, M, M_F, M_B);
					total_IO += workload*write_percentage*update_cost/100;
				}
			}
			if(read_percentage != 0 || rmw_percentage != 0 || blind_update_percentage != 0)
			{
				if(scenario == 'A') //average case
				{
					analyzeReadCostAvgCase(&avg_read_cost, &FPR_sum, T, K, Z, L, Y, M, M_B, M_BC, M_F, M_BF, data, -1);
					total_IO += workload*read_percentage*avg_read_cost/100;
				}
				else //worst case
				{
					analyzeReadCost(&read_cost, &FPR_sum, T, K, Z, L, Y, M, M_B, M_BC, M_F, M_BF, data);
					total_IO += workload*read_percentage*read_cost/100;
				}
			}
			if(rmw_percentage != 0)
			{
				rmw_cost = scenario == 'A'? avg_read_cost : read_cost;
				rmw_cost += (1.0/B);
				total_IO += workload*rmw_percentage*rmw_cost/100;
			}
			if(blind_update_percentage != 0)
			{
				blind_update_cost = scenario == 'A'? avg_read_cost : read_cost;
				blind_update_cost += (1.0/B);
				total_IO += workload*blind_update_percentage*blind_update_cost/100;
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
				(*bucket)->rmw_cost = rmw_cost;
				(*bucket)->blind_update_cost = blind_update_cost;
				(*bucket)->msg = "FASTER";
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
				if((*bucket)->total_cost == 0) // If system is any FASTER versions
				{
					(*bucket)->total_cost = 0.1;
				}
			}
    	}
	}
		
 	return 0;
}

