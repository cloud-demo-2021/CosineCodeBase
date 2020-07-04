#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>

#include "include/compare_designs.h"
#include "include/environment_variable.h"
#include "include/workload.h"

void getTieredLSMCost(design* d)
{
	double u_cost, r_cost, ss_cost, ls_cost, temp_cost;
	design* best = d, *down;
	tiered_LSM_cost = 99999;
	while(d)
	{	
		down = d;
		while(down)
		{
			u_cost = (double)d->L/(B);
			r_cost = 1.0 + d->FPR_sum*d->T; 
			ss_cost = (double)d->L*d->T;
			ls_cost = ceil(s/B)*d->T;
			temp_cost = (read_percentage*r_cost + write_percentage*u_cost + short_scan_percentage*ss_cost + long_scan_percentage*ls_cost)*query_count/100;
			if(temp_cost < tiered_LSM_cost)
			{
				tiered_LSM_cost = temp_cost;
				w_tiered_LSM_cost = u_cost;	
				r_tiered_LSM_cost = r_cost;
				best = d;
			}
			down = down->down;
		}
		d = d->next;
	}
	printf("\nTieredLSMCost: %f\t%f\t%f", w_tiered_LSM_cost, r_tiered_LSM_cost, tiered_LSM_cost);
	printf("\nTieredLSMCost: T=%d\tK=%d\tZ=%d\tM_f=%f\n", best->T, best->K, best->Z, best->M_F/(1024*1024*1024));
}

void getLazyLeveledLSMCost(design* d)
{
	double u_cost, r_cost, ss_cost, ls_cost, temp_cost;
	design* best = d;
	lazy_leveled_LSM_cost = 99999;
	while(d)
	{
		u_cost = ((double)d->L + d->T)/(B);
		r_cost = 1.0 + d->FPR_sum*d->T;
		ss_cost = 1.0 + (double)(d->L - 1)*d->T;
		ls_cost = ceil(s/B);
		temp_cost = (read_percentage*r_cost + write_percentage*u_cost + short_scan_percentage*ss_cost + long_scan_percentage*ls_cost)*query_count/100;
		if(temp_cost < lazy_leveled_LSM_cost)
		{
			lazy_leveled_LSM_cost = temp_cost;
			w_lazy_leveled_LSM_cost = u_cost;	
			r_lazy_leveled_LSM_cost = r_cost;
			best = d;
		}
		d = d->next;
	}
	printf("\nLazyLeveledLSMCost: %f\t%f\t%f", w_lazy_leveled_LSM_cost, r_lazy_leveled_LSM_cost, lazy_leveled_LSM_cost);
	printf("\nLAzyLeveledLSMCost: T=%d\tK=%d\tZ=%d\tM_f=%f\n", best->T, best->K, best->Z, best->M_F/(1024*1024*1024));
}

void getLeveledLSMCost(design* d)
{
	double u_cost, r_cost, ss_cost, ls_cost, temp_cost;
	design* best = d;
	leveled_LSM_cost = 99999;
	while(d)
	{
		u_cost = ((double)d->L * d->T)/(B);
		r_cost = 1.0 + d->FPR_sum;
		ss_cost = (double)d->L;
		ls_cost = ceil(s/B);
		temp_cost = (read_percentage*r_cost + write_percentage*u_cost + short_scan_percentage*ss_cost + long_scan_percentage*ls_cost)*query_count/100;
		if(temp_cost < leveled_LSM_cost)
		{
			leveled_LSM_cost = temp_cost;
			w_leveled_LSM_cost = u_cost;	
			r_leveled_LSM_cost = r_cost;
			best = d;
		}
		d = d->next;
	}
	printf("\nLeveledLSMCost: %f\t%f\t%f", w_leveled_LSM_cost, r_leveled_LSM_cost, leveled_LSM_cost);
	printf("\nLeveledLSMCost: T=%d\tK=%d\tZ=%d\tM_f=%f\n", best->T, best->K, best->Z, best->M_F/(1024*1024*1024));
}

void getCOLACost(design* d)
{
	double u_cost, r_cost, ss_cost, ls_cost, temp_cost;
	design* best = d;
	COLA_cost = 99999;
	while(d)
	{
		u_cost = ((double)d->L)/(B);
		r_cost = (double)d->L;
		ss_cost = (double)d->L;
		ls_cost = ceil(s/B);
		temp_cost = (read_percentage*r_cost + write_percentage*u_cost + short_scan_percentage*ss_cost + long_scan_percentage*ls_cost)*query_count/100;
		if(temp_cost < COLA_cost)
		{
			COLA_cost = temp_cost;
			w_COLA_cost = u_cost;	
			r_COLA_cost = r_cost;
			best = d;
		}
		d = d->next;
	}
	printf("\nCOLACost: %f\t%f\t%f", w_COLA_cost, r_COLA_cost, COLA_cost);
	printf("\nCOLACost: T=%d\tK=%d\tZ=%d\tM_f=%f\n", best->T, best->K, best->Z, best->M_F/(1024*1024*1024));
}

void getBEpsilonTreeCost(design* d)
{
	double u_cost, r_cost, ss_cost, ls_cost, temp_cost;
	design* best = d;
	b_epsilon_tree_cost = 99999;
	while(d)
	{
		u_cost = ((double)d->L*d->T)/(B);
		r_cost = (double)d->L;
		ss_cost = (double)d->L;
		ls_cost = ceil(s/B);
		temp_cost = (read_percentage*r_cost + write_percentage*u_cost + short_scan_percentage*ss_cost + long_scan_percentage*ls_cost)*query_count/100;
		if(temp_cost < b_epsilon_tree_cost)
		{
			b_epsilon_tree_cost = temp_cost;
			w_b_epsilon_tree_cost = u_cost;	
			r_b_epsilon_tree_cost = r_cost;
			best = d;
		}
		d = d->next;
	}
	printf("\nBepsilon-treeCost: %f\t%f\t%f", w_b_epsilon_tree_cost, r_b_epsilon_tree_cost, b_epsilon_tree_cost);
	printf("\nBepsilon-treeCost: T=%d\tK=%d\tZ=%d\tM_f=%f\n", best->T, best->K, best->Z, best->M_F/(1024*1024*1024));
}

void getBPlusTreeCost(design* d)
{
	double u_cost, r_cost, ss_cost, ls_cost, temp_cost;
	design* best = d;
	while(d)
	{
		b_plus_tree_cost = 99999;
		u_cost = ((double)d->L);
		r_cost = (double)d->L;
		ss_cost = (double)d->L;
		ls_cost = ceil(s/B);
		temp_cost = (read_percentage*r_cost + write_percentage*u_cost + short_scan_percentage*ss_cost + long_scan_percentage*ls_cost)*query_count/100;
		if(temp_cost < b_plus_tree_cost)
		{
			b_plus_tree_cost = temp_cost;
			w_b_plus_tree_cost = u_cost;	
			r_b_plus_tree_cost = r_cost;
			best = d;
		}
		d = d->next;
	}
	printf("\nB+-treeCost: %f\t%f\t%f", w_b_plus_tree_cost, r_b_plus_tree_cost, b_plus_tree_cost);
	printf("\nB+-treeCost: T=%d\tK=%d\tZ=%d\tM_f=%f\n", best->T, best->K, best->Z, best->M_F/(1024*1024*1024));
}

void compareWithOtherDesigns(design* d)
{
	getTieredLSMCost(d);
	getLazyLeveledLSMCost(d);
	getLeveledLSMCost(d);
	getCOLACost(d);
	getBEpsilonTreeCost(d);
	getBPlusTreeCost(d);
	printf("\nProposed design: %f\t%f\t%f\n", d->update_cost, d->read_cost, d->total_cost);
	printf("\nProposed design: T=%d\tK=%d\tZ=%d\tM_f=%f\n", d->T, d->K, d->Z, d->M_F/(1024*1024*1024));
}


