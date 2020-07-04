#include<stdio.h>
#include<stdlib.h>
#include <time.h>
#include<math.h>

#include "include/environment_variable.h"
#include "include/design_template.h"
#include "include/workload.h"
#include "include/user_sphere.h"
#include "include/compare_designs.h"
#include "include/VM_migration.h"
#include "include/index.h"


design* newDesign(int T, int K, int Z, int D, int L, int Y, double M, double M_B, double M_F, double M_F_HI, double M_F_LO, double M_FP, double M_BF, double FPR_sum, double update_cost, double read_cost, double short_scan_cost, double long_scan_cost, char* msg)
{
	design* newnode = (design*)malloc(sizeof(design));
	newnode->T = T;
	newnode->K = K;
	newnode->Z = Z;
	newnode->D = D;
	newnode->L = L;
	newnode->Y = Y;
	newnode->M = M;
	newnode->M_B = M_B;
	newnode->M_F = M_F;
	newnode->M_F_HI = M_F_HI;
	newnode->M_F_LO = M_F_LO;
	newnode->M_FP = M_FP;
	newnode->M_BF = M_BF;
	newnode->FPR_sum = FPR_sum;
	newnode->update_cost = update_cost;
	newnode->read_cost = read_cost;
	newnode->short_scan_cost = short_scan_cost;
	newnode->long_scan_cost = long_scan_cost;
	newnode->total_cost = (write_percentage*query_count*update_cost/100) + (read_percentage*read_cost*query_count/100) + (short_scan_percentage*short_scan_cost*query_count/100) + (long_scan_percentage*long_scan_cost*query_count/100);
	newnode->next = NULL;
	newnode->down = NULL;
	newnode->msg = msg;
    //printf("\nFPR_sum:%f", FPR_sum);
	return newnode;
}

void compressDesignsBasedOnSelectivity(design** d_list)
{
	design* temp = *d_list;
	design* comparison_point = temp;
	temp = temp->next;
	while(temp)
	{
		if(temp->total_cost < (1.0 + selectivity)*comparison_point->total_cost)
		{
			//printf("In IF Block because %f > %f\n", temp->total_cost, comparison_point->total_cost);
			comparison_point->next = temp->next;
			temp->down = comparison_point->down;
			comparison_point->down = temp;
			temp = comparison_point->next;
		}
		else
		{
			//printf("In ELSE Block\n");
			comparison_point = temp;
			temp = temp->next;
		}
	}
}

void showDesigns(design** d_list)
{
	int no_of_designs = 0, gap=0, d_id=0;
	design* temp = *d_list, *temp1 = NULL;
	while(temp)
	{
		
	 	//++gap; d_id++;
		//printf("\n%d. *************** Design: *********************\n", d_id);
		//printf("T=%d K=%d Z=%d D=%d L=%d Y=%d M=%f M_B=%f M_F_LO:%f M_F_HI:%f M_F=%f M_FP=%f M_BF=%f FPR_sum=%f update_cost:%f read_cost:%f total_cost:%f msg=%s\n", temp->T, temp->K, temp->Z, temp->D, temp->L, temp->Y, temp->M/(1024*1024*1024), temp->M_B/(1024*1024*1024), temp->M_F_LO/(1024*1024*1024), temp->M_F_HI/(1024*1024*1024), temp->M_F/(1024*1024*1024), temp->M_FP/(1024*1024*1024), temp->M_BF/(1024*1024*1024), temp->FPR_sum, temp->update_cost, temp->read_cost, temp->total_cost, temp->msg);
		//temp1 = temp->down;
		/*?while(temp1)
		{
			no_of_designs++;
			//printf("T=%d K=%d Z=%d D=%d L=%d Y=%d M=%f M_B=%f M_F_LO:%f M_F_HI:%f M_F=%f M_FP=%f M_BF=%f FPR_sum=%f update_cost:%f read_cost:%f total_cost:%f msg=%s\n", temp1->T, temp1->K, temp1->Z, temp1->D, temp1->L, temp1->Y, temp1->M/(1024*1024*1024), temp1->M_B/(1024*1024*1024), temp1->M_F_LO/(1024*1024*1024), temp1->M_F_HI/(1024*1024*1024), temp1->M_F/(1024*1024*1024), temp->M_FP/(1024*1024*1024), temp->M_BF/(1024*1024*1024), temp1->FPR_sum, temp1->update_cost, temp1->read_cost, temp1->total_cost, temp1->msg);
			temp1 = temp1->down;
		}*/
		//printf("\nTotal no: of designs: %d\n\n", no_of_designs);
		//printf("\n %f\t%f\t%f", temp->read_cost, temp->update_cost, temp->total_cost);
		printf("\n%f", temp->total_cost/(IOPS*3600*24));
		//printf("\n%f", temp->total_cost);
		/*if(gap++ == 20) 
		{
			printf("%f\t%f\n", temp->total_cost, temp->total_cost/(IOPS));
			printf("\n%f\t%f\t%f", temp->read_cost, temp->update_cost, (temp->total_cost/(IOPS))/3600);
			gap=0;
		}*/
		temp = temp->next;
		temp1 = NULL;
	}
}

void logUpdateCost(design** d_list, int T, int K, int Z, int L, int Y, double M, double M_B, double M_F, double M_F_HI, double M_F_LO, double M_FP, double M_BF, double update_cost, double read_cost, double short_scan_cost, double long_scan_cost, char* msg)
{
	//printf("\n%f\t%f", total_budget, update_cost);
	design* temp = *d_list, *temp1=NULL;
	design* previous = NULL;
	design* new_design = newDesign(T, K, Z, 1, L, Y, M, M_B, M_F, M_F_HI, M_F_LO, M_FP, M_BF, 0.0, update_cost, read_cost, short_scan_cost, long_scan_cost, msg);
	//printf("\nlogged %f", update_cost);
	if(!*d_list)
	{
		*d_list = new_design;
		return;
	}
	while(temp)
	{
		if(update_cost < temp->update_cost)
		{
			if(!previous) // insert deisgn at the beginning of the list
			{
				new_design->next = *d_list;
				*d_list = new_design;
			}
			else // insert deisgn somewhere at the middle of the list
			{
				previous->next = new_design;
				new_design->next = temp;
			}
			return;
		}
		else if(update_cost == temp->update_cost)
		{
			temp1 = temp;
			while(temp1->down)
			{
				temp1 = temp1->down;
			}
			temp1->down = new_design;
			//showDesigns(d_list);
			return;
		}
		previous = temp;
		temp = temp->next;
	}
	if(!temp) // insert at the end of the list
	{
		previous->next = new_design;
	}
}

void logReadCost(design** d_list, int T, int K, int Z, int L, int Y, double M, double M_B, double M_F, double M_F_HI, double M_F_LO, double M_FP, double M_BF, double FPR_sum, double update_cost, double read_cost, double short_scan_cost, double long_scan_cost, char* msg)
{
	design* temp = *d_list, *temp1=NULL;
	design* previous = NULL;
	design* new_design = newDesign(T, K, Z, 1, L, Y, M, M_B, M_F, M_F_HI, M_F_LO, M_FP, M_BF, FPR_sum, update_cost, read_cost, short_scan_cost, long_scan_cost, msg);
	if(!*d_list)
	{
		*d_list = new_design;
		return;
	}
	while(temp)
	{
		if(read_cost < temp->read_cost)
		{
			if(!previous) // insert deisgn at the beginning of the list
			{
				new_design->next = *d_list;
				*d_list = new_design;
			}
			else // insert deisgn somewhere at the middle of the list
			{
				previous->next = new_design;
				new_design->next = temp;
			}
			return;
		}
		else if(read_cost == temp->read_cost)
		{
			temp1 = temp;
			while(temp1->down)
			{
				temp1 = temp1->down;
			}
			temp1->down = new_design;
			//showDesigns(d_list);
			return;
		}
		previous = temp;
		temp = temp->next;
	}
	if(!temp) // insert at the end of the list
	{
		previous->next = new_design;
	}
}

void logTotalCost(design** d_list, int T, int K, int Z, int L, int Y, double M, double M_B, double M_F, double M_F_HI, double M_F_LO, double M_FP, double M_BF, double FPR_sum, double update_cost, double read_cost, double avg_read_cost, double no_result_cost, double short_scan_cost, double long_scan_cost, double avg_cpu_cost, double no_result_cpu_cost, char* msg)
{
	design* temp = *d_list, *temp1=NULL;
	design* previous = NULL;
	// should add in CPU cost
	if (Y == 1) {
		//printf("read cost: %f, avg_read_cost: %f, avg_cpu_cost:%f, FPR: %f, no_result_cost:%f, update_cost:%f, T: %d, K: %d, Z:%d, Y: %d, L:%d\n", read_cost, avg_read_cost, avg_cpu_cost, FPR_sum, no_result_cost, update_cost, T, K, Z, Y, L);
	}
	double read = 0;
	if (scenario == 'A') {
		read = avg_read_cost;
	}
	else {
		read = read_cost;
	}

	double total_cost = (write_percentage*query_count*update_cost/100) + (read_percentage*read*query_count/100) + (short_scan_percentage*short_scan_cost*query_count/100) + (long_scan_percentage*long_scan_cost*query_count/100);
	design* new_design = newDesign(T, K, Z, 1, L, Y, M, M_B, M_F, M_F_HI, M_F_LO, M_FP, M_BF, FPR_sum, update_cost, read, short_scan_cost, long_scan_cost, msg);
	if(!*d_list)
	{
		*d_list = new_design;
		return;
	}
	while(temp)
	{
		if(total_cost < temp->total_cost)
		{
			if(!previous) // insert deisgn at the beginning of the list
			{
				new_design->next = *d_list;
				*d_list = new_design;
			}
			else // insert deisgn somewhere at the middle of the list
			{
				previous->next = new_design;
				new_design->next = temp;
			}
			return;
		}
		else if(total_cost == temp->total_cost)
		{
			temp1 = temp;
			while(temp1->down)
			{
				temp1 = temp1->down;
			}
			temp1->down = new_design;
			return;
		}
		previous = temp;
		temp = temp->next;
	}
	if(!temp) // insert at the end of the list
	{
		previous->next = new_design;
	}
}




void getBestUpdateDesign(design** d_list)
{
	design* temp = *d_list;
	//double min_cost = temp->total_cost;
	printf("\nMinimum cost: %f\n", temp->update_cost);
	//printf("\nsecond term:%f\n", (double)(temp->T/temp->Z)*(temp->Y + 1));
	while(temp)
	{
		printf("T=%d K=%d Z=%d D=%d L=%d Y=%d M=%f M_B=%f M_F=%f M_F_HI=%f M_F_LO=%f update_cost:%f read_cost:%f total_cost:%f msg=%s\n", temp->T, temp->K, temp->Z, temp->D, temp->L, temp->Y, temp->M/(1024*1024*1024), temp->M_B/(1024*1024*1024), temp->M_F/(1024*1024*1024), temp->M_F_HI/(1024*1024*1024), temp->M_F_LO/(1024*1024*1024), temp->update_cost, temp->read_cost, temp->total_cost, temp->msg);
		//printf("\nlog(M_F_HI/M_F):%f log(T):%f", log(temp->M_F_HI/temp->M_F), log(temp->T));
		temp = temp->down;
	}
	printf("\n Total IOs:%f", (*d_list)->total_cost);
	printf("\n Total time taken to run workload:%f sec\n", (*d_list)->total_cost/(IOPS));
}

void getBestReadDesign(design** d_list)
{
	design* temp = *d_list;
	int td = 0;
	printf("\nMinimum cost: %f\n", temp->read_cost);
	while(temp)
	{
		printf("T=%d K=%d Z=%d D=%d L=%d Y=%d M=%f M_B=%f M_F_LO=%f M_F_HI=%f M_F=%f M_FP=%f M_BF=%f FPR_sum=%f update_cost:%f read_cost:%f total_cost:%f msg=%s\n", temp->T, temp->K, temp->Z, temp->D, temp->L, temp->Y, temp->M/(1024*1024*1024), temp->M_B/(1024*1024*1024), temp->M_F_LO/(1024*1024*1024), temp->M_F_HI/(1024*1024*1024), temp->M_F/(1024*1024*1024), temp->M_FP/(1024*1024*1024), temp->M_BF/(1024*1024*1024), temp->FPR_sum, temp->update_cost, temp->read_cost, temp->total_cost, temp->msg);
		temp = temp->down;
		td++;
	}
	printf("\ntotal designs:%d", td);
	printf("\n Total IOs:%f", (*d_list)->total_cost);
	printf("\n Total time taken to run workload:%f sec\n", (*d_list)->total_cost/(IOPS));
}

void getBestDesign(design** d_list)
{
	compressDesignsBasedOnSelectivity(d_list);
	int gap = 1, c=0; 
	design* temp = *d_list, *previous; 
	int td = 0; 
	double EB = (temp->M_B) / E;
	//double act_M_BF;
	//compareWithOtherDesigns(temp); 
	//printf("\n%f\t%f\t%f\t%f", read_percentage, temp->read_cost, temp->update_cost, temp->total_cost);
	//printf("\n%f\t%f", total_budget, temp->total_cost);
	//printf("T=%d K=%d Z=%d D=%d L=%d Y=%d M=%f M_B=%f M_F_LO=%f M_F_HI=%f M_F=%f M_FP=%f M_BF=%f EB=%f FPR_sum=%f update_cost:%f read_cost:%f total_cost:%f msg=%s\n", temp->T, temp->K, temp->Z, temp->D, temp->L, temp->Y, temp->M/(1024*1024*1024), temp->M_B/(1024*1024*1024), temp->M_F_LO/(1024*1024*1024), temp->M_F_HI/(1024*1024*1024), temp->M_F/(1024*1024*1024), temp->M_FP/(1024*1024*1024), temp->M_BF/(1024*1024*1024), EB, temp->FPR_sum, temp->update_cost, temp->read_cost, temp->total_cost, temp->msg);
	/*while(temp)
	{
		//if(c++ == gap) 
		{
			//printf("\n%f", temp->total_cost);
			printf("\n%f\t%f\t%f", temp->read_cost, temp->update_cost, temp->total_cost);
			c=0;
		}
		previous = temp;
		temp = temp->next;
	}*/

	printf("scenario:%c\n", scenario);
	if (workload_type == 0) {
		printf("N:%ld, U:%f, read_percentage:%f, write_percentage:%f, long_scan_percentage:%f\n", N, U, read_percentage, write_percentage, long_scan_percentage);
	}
	if (workload_type == 1) {
		printf("N:%ld, U_1:%f, U_2:%f, p_put:%f, p_get:%f, read_percentage:%f, write_percentage:%f, long_scan_percentage:%f\n", N, U_1, U_2, p_put, p_get, read_percentage, write_percentage, long_scan_percentage);
	}
	while(temp)
	{
		printf("T=%d K=%d Z=%d L=%d Y=%d M=%f M_B=%f M_F=%f M_FP=%f M_BF=%f FPR_sum=%f update_cost:%f read_cost:%f long_scan_cost:%f total_cost:%f msg=%s\n", temp->T, temp->K, temp->Z, temp->L, temp->Y, temp->M/(1024*1024*1024), temp->M_B/(1024*1024*1024), temp->M_F/(1024*1024*1024), temp->M_FP/(1024*1024*1024), temp->M_BF/(1024*1024*1024), temp->FPR_sum, temp->update_cost, temp->read_cost, temp->long_scan_cost, temp->total_cost, temp->msg);
		temp = temp->down;
	}
	//printf("\n Total IOs:%f", (*d_list)->total_cost);
	printf("\nTotal time taken to run workload using %s:%f sec (%f hour) (%f days)\n", pricing_scheme, (*d_list)->total_cost/(IOPS), ((*d_list)->total_cost/(IOPS))/3600, ((*d_list)->total_cost/(IOPS))/(24*3600));
	//getActualBloomFilterMemory(d_list, &act_M_BF);
	//printf("\nMemory wasted: %f", ((*d_list)->M_BF - act_M_BF)/(1024*1024*1024));
	//getTotalMigrationTime(d_list, 10);
}


void showDesignsSortByUpdateCost(design** d_list)
{
	int no_of_designs = 0;
	design* temp = *d_list, *temp1 = NULL;
	double min_read_cost = 1000000.0;
	while(temp)
	{
		no_of_designs = 1;
		min_read_cost = temp->read_cost;
		//printf("\n*************** Total cost: %f *********************\n", temp->total_cost);
		//printf("T=%d K=%d Z=%d D=%d L=%d Y=%d M=%f M_B=%f M_F=%f M_F_HI=%f M_F_LO=%f update_cost:%f read_cost:%f msg=%s\n", temp->T, temp->K, temp->Z, temp->D, temp->L, temp->Y, temp->M/(1024*1024*1024), temp->M_B/(1024*1024*1024), temp->M_F/(1024*1024*1024), temp->M_F_HI/(1024*1024*1024), temp->M_F_LO/(1024*1024*1024), temp->update_cost, temp->read_cost, temp->msg);
		temp1 = temp->down;
		while(temp1)
		{
			if(temp1->read_cost < min_read_cost)
				min_read_cost = temp1->read_cost;
			no_of_designs++;
			//printf("T=%d K=%d Z=%d D=%d L=%d Y=%d M=%f M_B=%f M_F=%f M_F_HI=%f M_F_LO=%f update_cost:%f read_cost:%f msg=%s\n", temp1->T, temp1->K, temp1->Z, temp1->D, temp1->L, temp1->Y, temp1->M/(1024*1024*1024), temp1->M_B/(1024*1024*1024), temp1->M_F/(1024*1024*1024), temp1->M_F_HI/(1024*1024*1024), temp1->M_F_LO/(1024*1024*1024), temp1->update_cost, temp1->read_cost, temp1->msg);
			temp1 = temp1->down;
		}
		printf("\n%f\t%f\t%f", ((p*temp->update_cost)+(g*min_read_cost)), temp->update_cost, min_read_cost);
		temp = temp->next;
		temp1 = NULL;
	}
}

void getActualBloomFilterMemory(design** d_list, double* act_M_BF)
{
	if((*d_list)->FPR_sum > 1)
	{
		*act_M_BF = (*d_list)->M_BF;
		return;
	}
	design* temp = *d_list;
	double e_estimate = 2.7182818;
	if(temp->FPR_sum < 0.0001) // ln(0.0001) = -9.210340
	{
		//printf("\nValue: %f", log(0.0001));
		*act_M_BF = (-1)*(N/8) * (-9.21034037198) /((2*log(2)/log(e_estimate)) * pow(temp->T, temp->Y) * pow(temp->Z, (temp->T-1)/temp->T) * pow(temp->K, 1/temp->T) * pow(temp->T, (temp->T/(temp->T-1)))/(temp->T-1));
	}
	else
	{
		*act_M_BF = (-1)*(N/8) * (log(temp->FPR_sum)/log(e_estimate)) /((2*log(2)/log(e_estimate)) * pow(temp->T, temp->Y) * pow(temp->Z, (temp->T-1)/temp->T) * pow(temp->K, 1/temp->T) * pow(temp->T, (temp->T/(temp->T-1)))/(temp->T-1));
	}
}


bool checkForCostReduction(design** d_list)
{
	double act_M_BF, mem_used, reduced_price;
	getActualBloomFilterMemory(d_list, &act_M_BF);
	mem_used = (*d_list)->M - ((*d_list)->M_BF - act_M_BF)/(1024.0*1024*1024); // in GB
	if(ceil((mem_used/(1024*1024*1024))/MIN_RAM_SIZE) >= 1)
	{
		reduced_price = ceil((mem_used/(1024*1024*1024))/MIN_RAM_SIZE)*monthlyPrice(RAM_BLOCK_COST);
	}
	printf("\nActual bloom filter: %f", act_M_BF/(1024.0*1024*1024));
	printf("\nReduced cost: $%f, Cost savings: $%f", reduced_price, total_budget-reduced_price);
	return false;
}











