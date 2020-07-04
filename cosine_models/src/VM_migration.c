#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>

#include "include/VM_migration.h"
#include "include/environment_variable.h"
#include "include/workload.h"

void initMigration()
{
	total_migration_time = 0;
	dirty_rate = 0.6*write_percentage/100; // 80% of writes are dirtied
}

void getTotalMigrationTime(design** d_list, int iteration)
{
	double act_M_BF, mem_used;
	getActualBloomFilterMemory(d_list, &act_M_BF);
	mem_used = (*d_list)->M - ((*d_list)->M_BF - act_M_BF);
	//printf("\nMem used: %f", mem_used/(1024*1024*1024));
	if(iteration == 1)
	{
		total_migration_time = mem_used/network_bandwidth;
	}
	else
	{
		// For the first iteration it is full memory copy. Second iteration onwards, it is based on dirty rate. 
		total_migration_time = (mem_used/network_bandwidth) + (iteration-1) * dirty_rate * (mem_used/network_bandwidth);
		stop_and_copy_time = dirty_rate * (mem_used/network_bandwidth);
		total_migration_time = total_migration_time + stop_and_copy_time;
		//printf("\n%f time taken for iteration %d", total_migration_time, iteration);
	}
}