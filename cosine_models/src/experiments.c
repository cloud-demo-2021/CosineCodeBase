#include "include/experiments.h"
#include "include/continuum.h"
#include "include/environment_variable.h"
#include "include/VM_library.h"
#include "include/workload.h"
#include "include/index.h"
#include "include/design_space_navigation.h"
#include "include/compression_library.h"
#include "include/SLA_factors.h"
#include "include/update_cost.h"
#include "include/read_cost.h"

#include <math.h>

void printContinuumExistingSystem()
{
	if(existing_system)
	{
		continuum_node* head = p_continuum, *down;
		int looking_for = 0, step = 5000;
		double to_be_showed;
		bool found = false;
		while(head && !found)
		{
			down = head;
			while(down)
			{
				if(down->provider_id == 0)
				{
					looking_for = down->cost - ((int)down->cost % step) + step;
					found = true;
					break;
				}
				down = down->next;
			}
			head = head->next;
		}
		head = p_continuum;
		while(head)
		{
			//printf("Looking for %d, got %f\n", looking_for, head->cost);
			if(head->cost <= looking_for && head->next && head->next->cost > looking_for)
			{
				//printf("Got into %d as I'm looking for %d\n", head->cost, looking_for);
				down = head;
			 	while(down)
			 	{
			 		//printf("Got: %f\t%f for %d\n", down->cost, down->latency, down->provider_id);
			 		if(down->provider_id == 0)
			 		{
			 			to_be_showed = ((int)(down->cost/step) + 1)*step;
			 			printf("%f\t%f\t%f\n", to_be_showed, down->mem_sum/(1024*1024*1024), down->latency/(3600));
						// printContinuumNode(down);
			 			looking_for += step;
			 			break;
			 		}
			 		down = down->next;
			 	}
			}
			head = head->next;
		}
	}
}

void printContinuumCosine(bool entire)
{
	if(!existing_system)
	{
		continuum_node* head = p_continuum, *temp, *best, *overall_best = p_continuum;
		double looking_for = 5000, step = 5000, to_be_showed;
		while(head)
		{
			if(head->cost <= looking_for && head->next && head->next->cost > looking_for)
			{
				if(entire)
				{
					temp = head;
					while(temp)
					{
						//printContinuumNode(temp);
						printf("%f\t%f (SLA)\t%f\t%f\t%s\t", temp->cost, temp->SLA_cost, temp->latency/(3600), temp->mem_sum/(1024*1024*1024), VM_libraries[temp->provider_id].provider_name);
						for(int i = 0;i<VM_libraries[temp->provider_id].no_of_instances;i++)
						{
							if(temp->VM_design_arr[i] != NULL)
							{
								printf("%s\n", temp->VM_design_arr[i]->msg);
							}
						}
					temp=temp->down;
					}
					printf("\n");
				}
				else
				{
					best = findBest(head);
					if(overall_best->latency < best->latency)
					{
						best = overall_best;
					}
					else
					{
						overall_best = best;
					}
					to_be_showed = ((int)(best->cost/step) + 1)*step;
					printf("%f\t%f (SLA)\t%f\t%s\t", to_be_showed, best->SLA_cost, best->latency/(3600), VM_libraries[best->provider_id].provider_name);
					for(int i = 0;i<VM_libraries[best->provider_id].no_of_instances;i++)
					{
						if(best->VM_design_arr[i] != NULL)
						{
							printf("%s ", best->VM_design_arr[i]->msg);
						}
					}
					// printContinuumNode(best);
					if(enable_performance_window)
					{
						for(int i = 0;i<VM_libraries[best->provider_id].no_of_instances;i++)
						{
							double update_cost = 0.0, avg_read_cost = 0.0, total_cost = 0.0;
							double FPR_sum=1.0;
							if(best->VM_design_arr[i] != NULL)
							{
								printf("A: %f %f ", best->VM_design_arr[i]->read_cost, best->VM_design_arr[i]->update_cost);
								getBestAndWorstCase(best->provider_id, best->VM_design_arr[i]->T, best->VM_design_arr[i]->K, best->VM_design_arr[i]->Z, best->VM_design_arr[i]->L, best->VM_design_arr[i]->Y, best->VM_design_arr[i]->M, best->VM_design_arr[i]->M_F, best->VM_design_arr[i]->M_B, best->VM_design_arr[i]->M_BF, best->VM_design_arr[i]->data_VM, best->VM_design_arr[i]->workload_VM);
							}
						}
					}
					printf("\n");	
				}
				looking_for += step;
			}
			head = head->next;
		}
	}
}


// Wacky optimizations not added here!
void getBestAndWorstCase(int provider_id, int T, int K, int Z, int L, int Y, double M, double M_F, double M_B, double M_BF, double data, double workload_VM)
{
	long double storage = ((double)(N)/(1024*1024*1024))*E;
	double MBps;
	if(provider_id == 0) // AWS
	{
		MBps = 3500;
		B = 256*1024/(E);
		IOPS = MBps*pow(10,6)/(B*E);
		if(IOPS > 15000) 
		{
			IOPS = 15000;
		}
	}
	else if(provider_id == 1) // GCP
	{
		MBps = read_percentage*720/100 + write_percentage*160/100; // taking average
		B = 16*1024/(E);
		IOPS = MBps*pow(10,6)/(B*E);
		if(IOPS > 30000) 
		{
			IOPS = 30000;
		}
	}
	else // AZURE
	{
		B = 8*1024/(E);
		if(storage <= 32)
		{
			IOPS = 120;
		}
		else if(storage > 32 && storage <= 64)
		{
			IOPS = 240;
		}
		else if(storage > 64 && storage <= 128)
		{
			IOPS = 500;
		}
		else if(storage > 128 && storage <= 256)
		{
			IOPS = 1100;
		}
		else if(storage > 256 && storage <= 512)
		{
			IOPS = 2300;
		}
		else if(storage > 512 && storage <= 2000)
		{
			IOPS = 5000;
		}
		else
		{
			IOPS = 7500;
		}
	}

	consider_partial_last_level = true;
	double FPR_sum=1.0;
	double update_cost = 0.0, avg_read_cost = 0.0, total_cost = 0.0;
	analyzeUpdateCostAvgCase(&update_cost, T, K, Z, L, Y, M, M_F, M_B, data, -1);
	analyzeReadCostAvgCase(&avg_read_cost, &FPR_sum, T, K, Z, L, Y, M, M_B, 0, M_F, M_BF, data, -1);
	total_cost = (write_percentage * workload_VM * update_cost/100) + (read_percentage * avg_read_cost * workload_VM/100);
	printf("B: %f %f %f ", avg_read_cost, update_cost, total_cost/(IOPS * 3600));
	consider_partial_last_level = false;

	consider_partial_last_level = true;
	consider_partial_buffer = true;
	update_cost = 0.0, avg_read_cost = 0.0, total_cost = 0.0;
	FPR_sum=1.0;
	analyzeUpdateCostAvgCase(&update_cost, T, K, Z, L, Y, M, M_F, M_B, data, -1);
	analyzeReadCostAvgCase(&avg_read_cost, &FPR_sum, T, K, Z, L, Y, M, M_B, 0, M_F, M_BF, data, -1);
	total_cost = (write_percentage * workload_VM * update_cost/100) + (read_percentage * avg_read_cost * workload_VM/100);
	//printf("W: %f %f %f ", avg_read_cost, update_cost, total_cost/(IOPS * 3600));
	consider_partial_buffer = false;
	consider_partial_last_level = false;

	/* ********************* Worst-case ************************ */
	update_cost = 0.0, avg_read_cost = 0.0, total_cost = 0.0;
	FPR_sum=1.0;
	if(T%32 == 0) // For B-trees avg-case and worst-case are close
	{
		analyzeUpdateCostAvgCase(&update_cost, T, K, Z, L, Y, M, M_F, M_B, data, -1);
		analyzeReadCostAvgCase(&avg_read_cost, &FPR_sum, T, K, Z, L, Y, M, M_B, 0, M_F, M_BF, data, -1);
	}
	else if (Z == 0 || Z == -1) // for LSH-tables, worst-case reads are close to avg-case
	{
		analyzeReadCostAvgCase(&avg_read_cost, &FPR_sum, T, K, Z, L, Y, M, M_B, 0, M_F, M_BF, data, -1);
		analyzeUpdateCostAvgCase(&update_cost, T, K, Z, L, Y, M, M_F, M_B, data, -1);
	}
	else
	{
		analyzeUpdateCost(&update_cost, T, K, Z, L, Y, M, M_F, M_B);
		analyzeReadCost(&avg_read_cost, &FPR_sum, T, K, Z, L, Y, M, M_B, 0, M_F, M_BF, data);
	}
	total_cost = (write_percentage * workload_VM * update_cost/100) + (read_percentage * avg_read_cost * workload_VM/100);
	printf("W: %f %f %f ", avg_read_cost, update_cost, total_cost/(IOPS * 3600));
}

