#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>

#include "include/workload.h"
#include "include/environment_variable.h"
#include "include/design_space_navigation.h"
#include "include/index.h"
#include "include/VM_migration.h"


void initWorkload()
{
	if(!wl_var_set)
	{
		N = 1000000000000;
		// for Uniform
		U = 100000000000000;
		p_put = 0.2; // fraction of the time that you call get on elements in U_1
		U_1 = 100000;
		U_2 = 1000000000000;
		// NOTE: it must always be true that (p_put / U_1) > (1 / U_2)
		p_get = 0.05; // fraction of the time that you call put on elements in U_1
		workload_type = 0; // 0 is uniform, 1 is skew

		query_count = 10000000000;
		read_percentage = 50; // point lookups
		write_percentage = 50; // inserts 
		rmw_percentage = 0; // read-modify writes 
		blind_update_percentage = 0; // blind updates
		short_scan_percentage = 0;
		long_scan_percentage = 0; // range queries
		long_scan_empty_percentage = 0; // empty range queries
	}
	no_of_windows = 1;
	change_percent = 30;
	s = 4096;
	head = NULL;
	workload_exec_time = 0;
	generateWorkloadOverTime();
	//showWorkloadWindows();
}

void generateWorkloadOverTime()
{
	long int so_far=0;
	srand((unsigned)time(NULL));
	windows = (window*)malloc(no_of_windows*sizeof(window));
	if(no_of_windows == 1)
	{
	 	windows[0].query_count = query_count;	
	 	windows[0].read_percentage = read_percentage;
	 	windows[0].write_percentage = write_percentage;
	 	return;
	}
	for(int i=0;i<no_of_windows;i++)
	{
		if(i == 0)
		{
			windows[i].query_count = query_count/no_of_windows; //rand()%query_count;
			windows[0].read_percentage = read_percentage;
	 		windows[0].write_percentage = write_percentage;
		}
		else
		{
			windows[i].query_count = query_count/no_of_windows; //rand()%(query_count - so_far);
			windows[i].read_percentage = windows[i-1].read_percentage + (rand()%2 == 0? 1: -1)*rand()%change_percent;
			if(windows[i].read_percentage < 0)
			{
				windows[i].read_percentage = 0;
			}
			if(windows[i].read_percentage > 100)
			{
				windows[i].read_percentage = 100;
			}
			windows[i].write_percentage = 100 - windows[i].read_percentage;
			if(windows[i].write_percentage < 0)
			{
				windows[i].write_percentage = 0;
			}
			if(windows[i].write_percentage > 100)
			{
				windows[i].write_percentage = 100;
			}
		}
		so_far = so_far + windows[i].query_count;
	}
}

void showWorkloadWindows()
{
	for(int i=0;i<no_of_windows;i++)
	{
		printf("\nWindow %d **************", i+1);
		printf("\nquery count: %ld read%%: %f write%%: %f", windows[i].query_count, windows[i].read_percentage, windows[i].write_percentage);
	}
}

void getM_B(double* M_B, int T, int L)
{
	*M_B = ((double)N*E)/pow(T, L);
}


void setOptimalStaticDesign()
{
	act_exec_time = d_list->total_cost;
	for(int i=1;i<no_of_windows;i++)
	{
		act_exec_time = act_exec_time + (windows[i].write_percentage*query_count*d_list->update_cost/100) 
										+ (windows[i].read_percentage*d_list->read_cost*query_count/100);
	}
	act_exec_time = (act_exec_time/(IOPS))/(3600*24);
}





