#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>

#include "include/read_cost.h"
#include "include/workload.h"
#include "include/environment_variable.h"

double getFPRSum(double M_BF, int T, int K, int Z, int L, int Y, double data) {
	return exp((-M_BF*8/data)*pow((log(2)/log(2.7182)), 2)*pow(T, Y)) * pow(Z, (T-1)/(double)T) * pow(K, 1/T) * pow(T, (T/(T-1)))/(T-1);
}

double sumOfEntriesRecursive(double M_B, int T, int L)
{
	double first = T*(M_B/E), sum = first;
	//if(L == 1)
	//	return T*(M_B/E);
	for(int i = 2;i<=L;i++)
	{
		sum = sum + first*pow(T, i-1);
	}
	return sum; 
}


double noOfEntries(int i, int T, int L, double data)
{
	return data*(T-1)/(pow(T, L-i-1));
}

double propLastlevelFull(int T, int K, int Z, int L, int Y, double M_B, double data, int C)
{

	double last_level;
	last_level = enable_CLL && C != -1? data*C/(C+1) : M_B*pow(T,L)/(E);
	double capacity_of_tree = sumOfEntriesRecursive(M_B, T, L);
	double prop_last_level_full = 1 - ((capacity_of_tree - data)/(last_level));
	if(prop_last_level_full > 1 || prop_last_level_full < 0)
	{
		prop_last_level_full = 1.0;
	}
		// printf("Total capcaity: %f, data: %f\n", capacity_of_tree, data);
		// printf("Last level is %f full\n", prop_last_level_full);
	return prop_last_level_full;
}


double getAlpha_i(int type, double M_B, double M_BC, int T, int K, int Z, int L, int Y, int i, double data, int C)
{
	// not a valid input 
	if (i < -1) return -1;

	// set up run size
	double size_run = 0;
	double p_skew = p_put;
	// block cache
	if (i == -1) {
		size_run = M_BC / E;
		// printf("block cache:%f\n", size_run);
		if (type == 1) size_run /= ((double) B);
		p_skew = p_get;
		// printf("block cache:%f, size_run:%f, p_skew:%f\n", size_run, M_BC, p_skew);
	}
	// buffer
	if (i == 0) {
		size_run = M_B / E;
		// IF WT then turn off M_B 
	}
	// hot levels except last
	if (i <= L - Y - 1 && i > 0) {
		if(enable_CLL && Y == 0 && C != -1)
		{
			size_run = M_B*pow(T,i)/(K*E*(C+1));
		}
		else
		{
			size_run = M_B*pow(T,i)/(K*E);	
		}
	}
	// last level
	if (i == L) {
		if(Z > 0) // other than LSH LSH-table
		{
			double prop_last_level_full = propLastlevelFull(T, K, Z, L, Y, M_B, data, C);
			if(consider_partial_last_level == false)
			{
				prop_last_level_full = 1.0; // Comment this line if you need to tailor the model to the situation where L=1
			}
			if(enable_CLL && Y == 0 && C != -1)
			{
				size_run = data * C / (Z * E * C+1);
			}
			else
			{
				size_run = prop_last_level_full*M_B*pow(T,i)/(Z*E);
			}
		}
		else // for Z = 0 and -1 in LSH table
		{
			size_run = M_B*pow(T,i)/(K*E);
		}
	}
	// cold levels
	if (i > 0 && i < L && i > L - Y -1) {
		size_run = M_B*pow(T,i)/(E);
		// size_run *= (B - T) / (double)B; // USE THIS LIN IF YOU WANT INTERNAL NODES TO STORE DATA
		size_run *= 0; // USE THIS LINE IF YOU DON'T WANT INTERNAL NODES TO STORE DATA

	}

	double prop_buffer_full = (data * E)/M_B;
	prop_buffer_full = prop_buffer_full - (int)prop_buffer_full;
	if(consider_partial_buffer == false)
	{
		prop_buffer_full = 1; // Comment this line if you need to tailor the model to the situation where L=1
		//printf("prop_buffer_full: %f\n", prop_buffer_full);
	}

	
	// get alpha
	if (type == 0) {
		double val = prop_buffer_full * size_run / U;
		//printf("val:%f\n", val);
		if(val < 1)
			return val;
		return 1;
	}
	if (type == 1) {
		double val = 1 - (p_skew / U_1);
		val = 1 - pow(val, size_run);
		// printf("val':%f\n", val);
		return val;
	}
	if (type == 2) {
		double val = (1 - p_skew) * size_run / U_2;
		if(val < 1)
			return val;
		return 1;
	}
	
	return -1;
}



void getcq(int type, int T, int K, int Z, int L, int Y, double M_B, double M_BC, double* c, double* q, double data, int C)
{
	*q = 1.0;
	for(int i=1;i<=L-Y-1;i++)
	{
		*q = (*q) * pow((1.0 - getAlpha_i(type, M_B, M_BC, T, K, Z, L, Y, i, data, C)), K);
	}
	int hot_level_boundary = L - Y > 1 ? L - Y : 1;
	for(int i=hot_level_boundary;i<=L;i++)
	{
		*q = (*q) * pow((1.0 - getAlpha_i(type, M_B, M_BC, T, K, Z, L, Y, i, data, C)), Z);
	}
	// double alpha_0 = getAlpha_i(type, M_B, M_BC, T, K, Z, L, Y, 0, data, C);
	*c = (1 - getAlpha_i(type, M_B, M_BC, T, K, Z, L, Y, -1, data, C)) * (1 - (*q))*(1 - getAlpha_i(type, M_B, M_BC, T, K, Z, L, Y, 0, data, C));
	*q = 1 - (*q)*(1 - getAlpha_i(type, M_B, M_BC, T, K, Z, L, Y, 0, data, C));
	//printf("c: %f, q: %f, alpha_0: %f\n", (*c), (*q), alpha_0);
}

double getC_ri(int type, int r, int i, double M_B, double M_BC, int T, int K, int Z, int L, int Y, double data, int C)
{
	double term1 = 1 - getAlpha_i(type, M_B, M_BC, T, K, Z, L, Y, 0, data, C);
	term1 *= (1 - getAlpha_i(type, M_B, M_BC, T, K, Z, L, Y, -1, data, C));
	//printf("Term1:%f\n", term1);
	double term2 = 1.0;
	for(int h=1;h<i;h++)
	{
		term2 = term2 * pow((1 - getAlpha_i(type, M_B, M_BC, T, K, Z, L, Y, h, data, C)), K);
	}
	double term3 = pow((1.0 - getAlpha_i(type, M_B, M_BC, T, K, Z, L, Y, i, data, C)), r);
	double term4 = 1;
	for(int h = i+1;h<=L-Y-1;h++)
	{
		term4 = term4 * pow((1 - getAlpha_i(type, M_B, M_BC, T, K, Z, L, Y, h, data, C)), K);
	}
	for(int h = L-Y;h<=L;h++)
	{
		if (h != 0) {
			term4 = term4 * pow((1 - getAlpha_i(type, M_B, M_BC, T, K, Z, L, Y, h, data, C)), Z);
		}
	}
	term4 = term4 * pow((1 - getAlpha_i(type, M_B, M_BC, T, K, Z, L, Y, i, data, C)), K-r);
	term4 = 1 - term4;
	return term1*term2*term3*term4;
}

double getD_ri(int type, int r, int i, double M_B, double M_BC, int T, int K, int Z, int L, int Y, double data, int C)
{
	double term1 = 1.0 - getAlpha_i(type, M_B, M_BC, T, K, Z, L, Y, 0, data, C);
	term1 *= (1.0 - getAlpha_i(type, M_B, M_BC, T, K, Z, L, Y, -1, data, C));
	// printf("term1:%f\n", term1);

	// printf("block cache term:%f\n", getAlpha_i(type, M_B, M_BC, T, K, Z, L, Y, -1));

	double term2 = 1.0;
	for(int h=1;h<=L-Y-1;h++)
	{
		term2 = term2 * pow((1 - getAlpha_i(type, M_B, M_BC, T, K, Z, L, Y, h, data, C)), K);
	}
	for(int h=L-Y;h<i;h++)
	{
		if (h != 0) {
			term2 = term2 * pow((1 - getAlpha_i(type, M_B, M_BC, T, K, Z, L, Y, h, data, C)), Z);
		}
	}
	double term3 = pow((1.0 - getAlpha_i(type, M_B, M_BC, T, K, Z, L, Y, i, data, C)), r);
	double term4 = 1.0;
	for(int h = i+1;h<=L;h++)
	{
		term4 = term4 * pow((1 - getAlpha_i(type, M_B, M_BC, T, K, Z, L, Y, h, data, C)), Z);
	}
	term4 = term4 * pow((1 - getAlpha_i(type, M_B, M_BC, T, K, Z, L, Y, i, data, C)), Z-r);
	term4 = 1 - term4;
	return term1*term2*term3*term4;
}

double aggregateAvgCase(int type, double* FPR_sum, int T, int K, int Z, int L, int Y, double M, double M_B, double M_BC, double M_F, double M_BF, double data, int C) {
	double term1 = 0.0, term2 = 0.0, term3= 0.0;
	double term2_2 = 0.0, term3_2 =0.0;
	double c, q;
	double p_i;
	getcq(type, T, K, Z, L, Y, M_B, M_BC, &c, &q, data, C);
	term1 = c/q;
	*FPR_sum = getFPRSum(M_BF, T, K, Z, L, Y, data);
	for(int i = 1;i<=L-Y-1;i++)
	{
		p_i = (*FPR_sum)*(T-1)/(T*K*pow(T, L-Y-i));
		if(enable_CLL && Y==0 && C != -1)
		{
			p_i = p_i / (C+1);
		}
		term2_2 = 0.0;
		for(int r = 1;r<=K;r++)
		{
			term2_2 = term2_2 + getC_ri(type, r, i, M_B, M_BC, T, K, Z, L, Y, data, C)/q;
		}
		term2 = term2 + (p_i * term2_2);
	}
	int hot_level_boundary = L - Y > 1 ? L - Y : 1;
	for(int i = hot_level_boundary;i<=L;i++)
	{
		if (i == L-Y) 
		{
			p_i = (*FPR_sum)*(T-1)/(T*Z);
			if(enable_CLL && Y==0 && C != -1)
			{
				p_i = p_i * C / (C+1);
			}
		}
		else {
			p_i = 1;
		}
		term3_2 = 0.0;
		for(int r = 1;r<=Z;r++)
		{
			term3_2 = term3_2 + (getD_ri(type, r, i, M_B, M_BC, T, K, Z, L, Y, data, C)/q);
			// printf("q:%f\n", q);
			// printf("Dri value:%f\n", getD_ri(type, r, i, M_B, M_BC, T, K, Z, L, Y)/q);
			// printf("Dri value:%f", getD_ri(type, r, i, M_B, M_BC, T, K, Z, L, Y));
		}
		term3 = term3 + (p_i * term3_2);
	}
	// printf("term1:%f\n", term1);
	// printf("term1:%f\n", term2);
	// printf("term3:%f\n", term3);
	return term1 + term2 + term3;
}

// This counts the number of bloom filters touched
double aggregateAvgCaseCPU(int type, int T, int K, int Z, int L, int Y, double M, double M_B, double M_BC, double M_F, double M_BF, int C) {
	double term1 = 0.0, term2 = 0.0, term3= 0.0;
	double term2_2 = 0.0;
	double c, q;
	getcq(type, T, K, Z, L, Y, M_B, M_BC, &c, &q, N, C); // N should be data actually - take care of it before using it
	term1 = c/q;
	for(int i = 1;i<=L-Y-1;i++)
	{
		term2_2 = 0.0;
		for(int r = 1;r<=K;r++)
		{
			term2_2 = term2_2 + getC_ri(type, r, i, M_B, M_BC, T, K, Z, L, Y, N, C)/q; // N should be data actually - take care of it before using it
		}
		term2 = term2 + term2_2;
	}

	term3 = 0.0;
	for(int r = 1;r<=Z;r++)
	{
		term3 = term3 + getD_ri(type, r, L - Y, M_B, M_BC, T, K, Z, L, Y, N, C)/q; // N should be data actually - take care of it before using it
	}
	return term1 + term2 + term3;
}

void analyzeReadCost(double* read_cost, double* FPR_sum, int T, int K, int Z, int L, int Y, double M, double M_B, double M_BC, double M_F, double M_BF, double data)
{
	if(Z == 0 || Z == -1)
	{
		*read_cost = 1;
		return;
	}
	if (Y == L) {
		*read_cost = Y * Z;
		return;
	}
	double entries_in_hot_level=0.0;
	entries_in_hot_level = sumOfEntriesRecursive(M_B, T, L-Y);
	double bits_per_entry = M_BF*8/entries_in_hot_level;
	*FPR_sum = getFPRSum(M_BF, T, K, Z, L, Y, data);
	*read_cost =  1.0 + (Y*Z) + (*FPR_sum);
}

void analyzeNoResultCost(double* no_result_read_cost, double* FPR_sum, int T, int K, int Z, int L, int Y, double M, double M_B, double M_BC, double M_F, double M_BF, double data)
{
	double entries_in_hot_level=0.0;
	entries_in_hot_level = sumOfEntriesRecursive(M_B, T, L-Y);
	double bits_per_entry = M_BF*8/entries_in_hot_level;
	*FPR_sum = getFPRSum(M_BF, T, K, Z, L, Y, data);
	*no_result_read_cost =  (Y*Z) + (*FPR_sum);
}

void analyzeNoResultCPU(double* no_result_cpu_cost, int T, int K, int Z, int L, int Y, double M, double M_B, double M_BC, double M_F, double M_BF)
{
	double entries_in_hot_level=0.0;
	entries_in_hot_level = sumOfEntriesRecursive(M_B, T, L-Y);
	double bits_per_entry = M_BF*8/entries_in_hot_level;
	*no_result_cpu_cost =  (L - Y - 1) * K + Z;
}

void analyzeReadCostAvgCase(double* avg_read_cost, double* FPR_sum, int T, int K, int Z, int L, int Y, double M, double M_B, double M_BC, double M_F, double M_BF, double data, int C)
{
	if(Z == 0) // LSH-table append-only
	{
		double scale_up = 1.8;
		double term1;
		double c, q;
		double alpha_0 = getAlpha_i(workload_type, M_B, M_BC, T, K, Z, L, Y, 0, data, C);
		//printf("alpha_0: %f\n", alpha_0);
		q = pow((1.0 - getAlpha_i(workload_type, M_B, M_BC, T, K, Z, L, Y, 1, data, C)), K);
		c = (1 - q)*(1 - getAlpha_i(workload_type, M_B, M_BC, T, K, Z, L, Y, 0, data, C));
		q = 1 - q*(1 - getAlpha_i(workload_type, M_B, M_BC, T, K, Z, L, Y, 0, data, C)); 
		term1 = c;
		//printf("in DS: %f on disk: %f, term1: %f, M_B:%f\n", q, c, term1, M_B/(1024*1024*1024));
		*avg_read_cost = term1*scale_up;
		return;
	}
	else if (Z == -1) // LSH-table hybrid logs
	{
		double term1;
		double c, q;
		double alpha_mutable = getAlpha_i(workload_type, 0.9*M_B, M_BC, T, K, Z, L, Y, 0, data, C);
		double alpha_read_only = getAlpha_i(workload_type, 0.1*M_B, M_BC, T, K, Z, L, Y, 0, data, C);	
		double alpha_0 = 1 - ((1 - alpha_mutable) * (1 - alpha_read_only));
		q = pow((1.0 - getAlpha_i(workload_type, M_B, M_BC, T, K, 1, L, Y, 1, data, C)), K);
		c = (1 - q)*(1 - alpha_0);
		q = 1 - q*(1 - alpha_0); 
		term1 = c;
		//printf("in DS: %f on disk: %f\n", q, c);
		//printf("in DS: %f on disk: %f, term1: %f, M_B:%f\n", q, c, term1, M_B/(1024*1024*1024));
		*avg_read_cost = term1;
		return;
	}
	// uniform
	if (workload_type == 0) {
		*avg_read_cost = aggregateAvgCase(0, FPR_sum, T, K, Z, L, Y, M, M_B, M_BC, M_F, M_BF, data, C);
		return;
	}

	// skew
	if (workload_type == 1) {
		double skew_part =  aggregateAvgCase(1, FPR_sum, T, K, Z, L, Y, M, M_B, M_BC, M_F, M_BF, data, C);
		double non_skew_part =  aggregateAvgCase(2, FPR_sum, T, K, Z, L, Y, M, M_B, M_BC, M_F, M_BF, data, C);
		*avg_read_cost = skew_part * p_get + non_skew_part * (1 - p_get);
	}
	return; 
}

void analyzeReadCostAvgCaseCPU(double* avg_read_cost_cpu, int T, int K, int Z, int L, int Y, double M, double M_B, double M_BC, double M_F, double M_BF, int C)
{
	// uniform
	if (workload_type == 0) {
		*avg_read_cost_cpu = aggregateAvgCaseCPU(0, T, K, Z, L, Y, M, M_B, M_BC, M_F, M_BF, C);
		return;
	}

	// skew
	if (workload_type == 1) {
		double skew_part =  aggregateAvgCaseCPU(1, T, K, Z, L, Y, M, M_B, M_BC, M_F, M_BF, C);
		double non_skew_part =  aggregateAvgCaseCPU(2, T, K, Z, L, Y, M, M_B, M_BC, M_F, M_BF, C);
		*avg_read_cost_cpu = skew_part * p_get + non_skew_part * (1 - p_get);
	}
	return; 
}








