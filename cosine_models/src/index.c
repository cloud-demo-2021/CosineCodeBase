// compile with gcc -std=c99 -o index *.c -lm on adama

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include <string.h>
#include <time.h>
#include <assert.h> 

#include "include/environment_variable.h"
#include "include/design_template.h"
#include "include/update_cost.h"
#include "include/read_cost.h"
#include "include/design_space_navigation.h"
#include "include/index.h"
#include "include/user_sphere.h"
#include "include/workload.h"
#include "include/VM_library.h"
#include "include/continuum.h"
#include "include/compression_library.h"
#include "include/SLA_factors.h"
#include "include/experiments.h"

double monthlyPrice(double hourly_price)
{
	return 24*30*hourly_price;
}

void getTotalNoOfDesigns(long int* td)
{
	*td = *td + (B-1)*B*B*((double)N/B);
}

int getMaxInt(int a, int b)
{
	return (a > b? a : b);
}

void setPricesBasedOnScheme(double cost)
{
	total_budget = cost; //generateRandomBudget(MIN_BUDGET, MAX_BUDGET); for a month
	float storage, MBps, monthly_storage_cost;
	storage = (N*E)/(1024*1024*1024);
	if(strcmp(pricing_scheme, "AWS") == 0)
	{
		if(enable_SLA == 1)
		{
			total_budget = total_budget - DB_migration_cost_AWS*storage;
		}
		//printf("Budget to purchase resources: %f\n", total_budget);
		MIN_RAM_SIZE = 16; // GB
		RAM_BLOCK_COST = 0.091; // per RAM block per hour
		MBps = 3500; // it is actually Mbps  for AWS
		B = 256*1024/(E); //https://docs.aws.amazon.com/AWSEC2/latest/UserGuide/memory-optimized-instances.html
		IOPS = MBps*pow(10,6)/(B*E);
		if(IOPS > 15000) 
		{
			IOPS = 15000;
		}
		if(storage > 75)
		{
			monthly_storage_cost = (storage-75)*0.1; // $0.1 per GB-month https://aws.amazon.com/ebs/pricing/
			total_budget = total_budget - monthly_storage_cost;
		}
		else
		{
			monthly_storage_cost = storage*0.1; // $0.1 per GB-month https://aws.amazon.com/ebs/pricing/
		}
		network_bandwidth = 10.0*1024*1024*1024/8; //Gbps
		//printf("\ndone************ %f", network_bandwidth);
	}
	if(strcmp(pricing_scheme, "GCP") == 0)
	{
		if(enable_SLA == 1)
		{
			total_budget = total_budget - DB_migration_cost_GCP*storage;
		}
		MIN_RAM_SIZE = 13; // GB
		RAM_BLOCK_COST = 0.0745; // per RAM block per hour
		MBps = read_percentage*720/100 + write_percentage*160/100; // taking average
		B = 16*1024/(E);
		IOPS = MBps*pow(10,6)/(B*E);
		if(IOPS > 30000) 
		{
			IOPS = 30000;
		}
		monthly_storage_cost = storage*0.17;
		total_budget = total_budget - monthly_storage_cost;
	}
	if(strcmp(pricing_scheme, "AZURE") == 0)
	{
		if(enable_SLA == 1)
		{
			total_budget = total_budget - DB_migration_cost_Azure*storage;
		}
		MIN_RAM_SIZE = 16; // GB
		RAM_BLOCK_COST = 0.0782; // per RAM block per hour
		B = 8*1024/(E);
		if(storage <= 32)
		{
			IOPS = 120;
			monthly_storage_cost = 5.28;
		}
		else if(storage > 32 && storage <= 64)
		{
			IOPS = 240;
			monthly_storage_cost = 10.21;
		}
		else if(storage > 64 && storage <= 128)
		{
			IOPS = 500;
			monthly_storage_cost = 19.71;
		}
		else if(storage > 128 && storage <= 256)
		{
			IOPS = 1100;
			monthly_storage_cost = 38.02;
		}
		else if(storage > 256 && storage <= 512)
		{
			IOPS = 2300;
			monthly_storage_cost = 73.22;
		}
		else if(storage > 512 && storage <= 2000)
		{
			IOPS = 5000;
			monthly_storage_cost = 135.17;
		}
		else
		{
			IOPS = 7500;
			monthly_storage_cost = 259.05;
		}
		total_budget = total_budget - monthly_storage_cost;		
	}
}

void setMaxRAMNeeded() 
{
	if(total_budget <= 0)
	{
		printf("\n************ INSUFFICIENT BUDGET FOR %s PRICING SCHEME *************\n", pricing_scheme);
		exit(0);
	}
	int max_RAM_blocks = (int)total_budget/monthlyPrice(RAM_BLOCK_COST);
	if(max_RAM_blocks < 1)
	{
		printf("\n************ INSUFFICIENT BUDGET FOR %s PRICING SCHEME *************\n", pricing_scheme);
		exit(0);
	}
	max_RAM_needed = (((double)N*E)/(1024.0*1024*1024)); // in GB
	if(MIN_RAM_SIZE*max_RAM_blocks <= max_RAM_needed) // what I can purchase is less than or equal to what I need
	{
		max_RAM_purchased = MIN_RAM_SIZE*max_RAM_blocks;
	}
	else // what I can purchase is more than what I need
	{
		max_RAM_purchased = ceil(max_RAM_needed/MIN_RAM_SIZE)*MIN_RAM_SIZE;
	}
	//printf("\nmax_RAM_needed:%f \tmax_RAM_purchased:%f", max_RAM_needed, max_RAM_purchased);
}

void initParameters(double cost)
{
	setPricesBasedOnScheme(cost);
}



int FitsInMemory(double M, double data)
{
	if(M >= data*E)
		return 1;
	else
		return 0;
}


void runSystem()
{
	double M_B; 
	double M_B_percent;
	//max_RAM_purchased = 32.0;

	for(int i=0;i<no_of_windows;i++)
	{
		M_B_percent=20;
		M_B = (double)M_B_percent*max_RAM_purchased/100;
		d_list = NULL; 
		query_count = windows[i].query_count;	
	 	read_percentage = windows[i].read_percentage;
	 	write_percentage = windows[i].write_percentage; 
	 	if(FitsInMemory(max_RAM_purchased*1024*1024*1024, N))
		{
			printf("\nTotal data volume %f fits in %f GB memory", ((double)N*E)/(1024*1024*1024), max_RAM_purchased);
			logUpdateCost(&d_list, 0, 0, 0, 0, 0, max_RAM_purchased, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, "Fits in memory");
		}
		while(M_B < max_RAM_purchased)
		{
			navigateDesignSpace1(max_RAM_purchased*1024*1024*1024, M_B*1024*1024*1024, &d_list, "LSM"); 
			navigateDesignSpace1(max_RAM_purchased*1024*1024*1024, M_B*1024*1024*1024, &d_list, "BTREE"); 
			M_B_percent = M_B_percent + 20;
			M_B = (double)M_B_percent*max_RAM_purchased/100;
		}
		if(i == 0)
		{
			
			setOptimalStaticDesign();
		}
		//compressDesignsBasedOnSelectivity(&d_list);
		//workload_exec_time = workload_exec_time + (d_list->total_cost/(IOPS))/(3600*24);
		//printf("\nWindow executed in %f days", (d_list->total_cost/(IOPS))/(3600*24));
		//showDesigns(&d_list);
		getBestDesign(&d_list);
		//checkForCostReduction(&d_list);
		//printf("%f\n", d_list->total_cost/(IOPS));
		free(d_list);
	}
	//printf("\nWorkload executed in %f days", workload_exec_time);
	//printf("\nWorkload executed in %f days with a static design", act_exec_time);
}

//******************************* main() begins ***************************

int main(int argc, char* argv[])
{
	p_continuum = NULL;
	existing_system = NULL;
	wl_var_set = false;
	if(execution_mode == 'C') // continuum mode
	{
		if(argc != 1)
		{
			existing_system = argv[1];
			printf("System: %s\n", existing_system);
		}
		clock_t start, end;
     	double cpu_time_used;
     	start = clock();
		initializeVMLibraries();
		initializeSLAFactors();
		initializeCompressionLibraries();
		initWorkload();
		getAllVMCombinations();
		buildContinuum(); 
		correctContinuum(); 
	    end = clock();
	    cpu_time_used = ((double) (end - start)) / CLOCKS_PER_SEC; // in second
	    printf("\nContinuum built in %f second.\n",  cpu_time_used);
	    //printContinuum(false);
	    //printContinuumAtGap();

	    // ***************** Experiments Block Begin ***************** 
	    printContinuumExistingSystem();
	    printContinuumCosine(false);
	    // ***************** Experiments Block End ***************** 

     	free(p_continuum);
	}
	else if(execution_mode == 'N') // normal mode
	{
		double cost = 40000;
		if(argc == 1)
		{
			pricing_scheme = "AWS";
		}
		else
		{
			pricing_scheme = argv[1];
		}

		initWorkload();
		initializeSLAFactors();
		initParameters(cost);
		setMaxRAMNeeded();
		runSystem();
	}
	else
	{
		// ./build/a.out AWS 40000 1000000000000 0 50 50 0 10000000000 100000000000000
		// ./build/a.out AWS 40000 1000000000000 1 50 50 0 10000000000 100000000000000 10000000000 0.1 0.5
		printf("\n************** Inside Command Line Argument Mode **************\n");
		if (argc < 10) 
		{
	      fprintf(stderr, "Usage:<cloud provider>, <#budget>, <#data>, <workload_type>, <get%%>, <put%%>, <scan%%>, <#queries>, <U or U_1,U_2,p_put,p_get>\n");
	      exit(1);
	    }
	    pricing_scheme = argv[1];
	    double cost = atof(argv[2]);
	    N = atol(argv[3]);
	    workload_type = atoi(argv[4]); 

		read_percentage = atof(argv[5]);
		write_percentage = atof(argv[6]);
		//short_scan_percentage = atof(argv[6]);
		long_scan_percentage = atof(argv[7]); 
		query_count = atol(argv[8]); 

		if(workload_type == 0)
		{
			U = atof(argv[9]); 
		}
		else
		{
			U_1 = atof(argv[9]); 
			U_2 = atof(argv[10]);
	    	p_put = atof(argv[11]);
	    	p_get = atof(argv[12]);
		}

    	wl_var_set = true;

    	initWorkload();
		initializeSLAFactors();
		initParameters(cost);
		setMaxRAMNeeded();
		runSystem();
	}
	return 0;
}

//******************************* main() ends ***************************
