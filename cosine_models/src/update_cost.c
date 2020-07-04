#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include <float.h>

#include "include/update_cost.h"
#include "include/workload.h"
#include "include/environment_variable.h"
#include "include/read_cost.h"


// coupon collector formula for obtaining number_of_entries elements from uniform(universe)
double getCouponCollector(double universe, double number_of_entries) {
	// printf("number_of_entries:%f\n", number_of_entries);
	if (number_of_entries == 1) return 1;
	if (number_of_entries > universe) return -1;
	double ratio = ((double) universe) / ((double) (universe - number_of_entries + 1));
	return universe * log(ratio);
}

double getQ(int type, int level, double EB, int T, int K, int worst_case) {
	// uniform
	double size_run = EB;
	double worst_case_estimate = size_run;
	if (level != 0) {
		size_run = ((double) (EB * pow(T, level)))/ ((double) K);

		worst_case_estimate = size_run * K;
	}



	if (worst_case) return worst_case_estimate;

	double avg_case_bound = 0;
	if (type == 0) {

		avg_case_bound = getCouponCollector(U, size_run);
		if (level != 0) avg_case_bound *= K;
	}
	// skew
	if (type == 1) {
		double bound_1 = DBL_MIN;
		double bound_2 = getCouponCollector(U_2, size_run);
		// bound 1: some special key slots are not filled up
		if (level != 0) bound_2 *= K;

		// bound 2: all special key slots are filled up
		if (size_run > U_1) {
			bound_1 = getCouponCollector(U_2, size_run - U_1) / (1 - p_put);
			if (level != 0) bound_1 *= K;
		}

		// printf("bound 1: %f, bound 2:%f\n", bound_1, bound_2);
		avg_case_bound = (bound_2 >= bound_1) ? bound_2 : bound_1;
	}

	return (avg_case_bound <= worst_case_estimate) ? worst_case_estimate : avg_case_bound;
}

double aggregateAvgCaseUpdate(int type, int T, int K, int Z, int L, int Y, double M_B, int worst_case) {
	double EB = M_B / ((double) E);
	double term1 = 0.0, term2 = 0.0, term3= 0.0, term3_2 = 0.0, term3_mult = 0.0;

	for(int i = 1;i<=L - 1 - Y;i++)
	{
		double numerator = ((double) (EB * pow(T, i)))/ ((double) K); // run in next level
		// double numerator = 0; // don't count run in next level
		double numerator_2 = EB * pow(T, i - 1); // all of previous level
		// double numerator_2 = 0; // don't count this level
		double Q  = getQ(type, i - 1, EB, T, K, worst_case);
		if (Q > 0) {
			term1 += (numerator + numerator_2) / Q;
		}	
	}
	term1 /= ((double) B);

	term2 = ((double) (EB * pow(T, L - Y)))/ ((double) Z) + EB * pow(T, L - 1 - Y);
	// term2 =  ((double) (EB * pow(T, L - Y)))/ ((double) Z); // don't count this level
	// term2 =  EB * pow(T, L - Y - 1); // don't count next level
	term2 /= ((double) B);
	double Q = getQ(type, L - 1 - Y, EB, T, K, worst_case);
	// printf("L:%d, Y:%d", L, Y);
	// printf("Q:%f\n", Q);
	if (Q < 0) {
		term2 = 0;
	}
	else {
		term2 /= Q;
		if (Y >= 1) {
			term3_mult = 1.0;
			if (T < B) {
				term3_mult = T <= B-T ? T : B - T;
				term3_mult /= ((double) (B - T));
			}
			for(int i = L - Y + 1; i <= L ;i++)
			{
				double num_blocks = ((double) (EB * pow(T, i)))/ ((double) B);
				term3_2 = EB * pow(T, L-Y-1) * term3_mult >= num_blocks ? num_blocks : EB * pow(T, L - 1 - Y) * term3_mult;
				term3 += term3_2;
			}
			term3 /= Q;
		}
	}

	// printf("term1:%f, term2:%f, term3:%f\n", term1, term2, term3);
	// printf("T:%d, K:%d, Z:%d, L:%d, Y:%d, MB:%f, EB:%f, N:%ld\n", T, K, Z, L, Y, M_B, EB, N);

	return term1 + term2 + term3;
}

void analyzeUpdateCostAvgCase(double* update_cost, int T, int K, int Z, int L, int Y, double M, double M_F, double M_B, double data)
{
	if(Z == 0) // LSH-table append-only
	{
		double scale_up = 1.5;
		double term1;
		double c, q;
		q = pow((1.0 - getAlpha_i(workload_type, M_B, 0.0, T, K, Z, L, Y, 1, data)), K);
		c = (1 - q)*(1 - getAlpha_i(workload_type, M_B, 0.0, T, K, Z, L, Y, 0, data));
		q = 1 - q*(1 - getAlpha_i(workload_type, M_B, 0.0, T, K, Z, L, Y, 0, data)); 
		term1 = c/q;
		//printf("in DS: %f on disk: %f\n", q, c);
		*update_cost = term1 *  scale_up;
		return;
	}
	else if (Z == -1) // LSH-table hybrid logs
	{
		//printf("Hybrid log in FASTER\n");
		double term1;
		double c, q;
		double alpha_mutable = getAlpha_i(workload_type, 0.9*M_B, 0.0, T, K, Z, L, Y, 0, data);
		double alpha_read_only = getAlpha_i(workload_type, 0.1*M_B, 0.0, T, K, Z, L, Y, 0, data);	
		double alpha_0 = 1 - ((1 - alpha_mutable) * (1 - alpha_read_only));
		//printf("alpha_mutable: %F, alpha_read_only: %f, alpha_0: %f\n", alpha_mutable, alpha_read_only, alpha_0);
		q = pow((1.0 - getAlpha_i(workload_type, M_B, 0.0, T, K, 1, L, Y, 1, data)), K);
		c = (1 - q)*(1 - alpha_0);
		q = 1 - q*(1 - alpha_0); 
		term1 = c/q;
		//printf("in DS: %f on disk: %f\n", q, c);
		*update_cost = term1;
		return;
	}
	*update_cost = aggregateAvgCaseUpdate(workload_type, T, K, Z, L, Y, M_B, 0);
	
}

void analyzeUpdateCost(double* update_cost, int T, int K, int Z, int L, int Y, double M, double M_F, double M_B)
{
	if (Z == 0 || Z == -1)
	{
		*update_cost = 1.0/B;
		return;
	}
	// double update_cost_hot_levels = (double)(T*(L-Y-1))/K;
	// double update_cost_cold_levels = (double)(T/Z)*(Y+1);
	// *update_cost = (update_cost_hot_levels + update_cost_cold_levels)/B; 
	*update_cost = aggregateAvgCaseUpdate(workload_type, T, K, Z, L, Y, M_B, 1);

}
