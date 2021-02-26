#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>

#include "include/parameter_computation.h"
#include "include/workload.h"
#include "include/environment_variable.h"


void getNoOfLevelsWacky(int* L, double M_B, int T, double data, int C)
{	
	if(M_B == 0)
	{
		printf("ERROR!!!! Buffer memory should never be set to 0.");
		*L = 0; // This is not really true. If 
	}
	else
	{
		double multiplier_from_buffer = data*((double)E) / (M_B);
		// handle case where data fits in buffer
		if (multiplier_from_buffer < 1) multiplier_from_buffer = 1;
		*L = (int)ceil(log(multiplier_from_buffer * (T-1) / (C+1))/log(T));
	}
}


void getNoOfLevels(int* L, double M_B, int T, double data)
{	
	if(M_B == 0)
	{
		printf("ERROR!!!! Buffer memory should never be set to 0.");
		*L = 0; // This is not really true. If 
	}
	else
	{
		double multiplier_from_buffer = data*((double)E) / (M_B);
		// handle case where data fits in buffer
		if (multiplier_from_buffer < 1) multiplier_from_buffer = 1;
		*L = (int)ceil(log(multiplier_from_buffer)/log(T));
		//printf("N:%d E:%d M_B:%f T:%d L:%d\n", N, E, M_B, T, *L);
	}
}


void getNoOfLevelsAvgCase(int* L, double M_B, int T, double data)
{	
	double universe_max = workload_type == 0 ? U : U_1 + U_2;
	if (workload_type == 1) {
		universe_max = U_1 + (1 - p_put) * (data);	
	}
	double size = universe_max < data ? universe_max : data;
	getNoOfLevels(L, M_B, T, size);
}

void getX(double* X, int T, int K, int Z)
{
	*X = (((double)log(T)/(T-1)) + (double)(double)(log(K) - log(Z))/T)/(2*log(2))*8; // convert to bits
	*X = (((double)log(T)/(T-1)) + (double)(double)(log(K) - log(Z))/T)/(pow(log(2),2))*8; // convert to bits
	if(*X < 0)
		*X = 0;
}

void getY(int* Y, double* M_FP, double M_F, double M_F_HI, double M_F_LO, double X, int T, int L, double data)
{
	int i, h, c;
	double temp_M_FP, act_M_FP;
	if(M_F >= M_F_HI)
	{
		c=1;
		*Y = 0;
		*M_FP = data*(F)/(B);
	}
	else if(M_F > M_F_LO && M_F < M_F_HI)
	{
		c=2;
		*Y = L-1;
		*M_FP = M_F_LO;
		for(i=L-2;i>=1;i--)
		{
			h = L - i;
			temp_M_FP = M_F_LO;
			for(int j = 2;j<=h; j++)
			{
				temp_M_FP = temp_M_FP + (temp_M_FP*T);
			}
			if(temp_M_FP <= M_F)
			{
				*Y = i;
				*M_FP = temp_M_FP;
			}
		}		
	}
	else
	{
		c=3;
		*Y = L-1;
		*M_FP = M_F_LO;
	}
}

void getM_FP(double* M_FP, int T, int L, int Y, double M_B, double M_F_LO)
{
	int h;
	double temp_M_FP;
	if(Y == L-1)
		*M_FP = (double)(M_B*(F)*T)/((B)*(E));
	else
	{
		h = L - Y;
		temp_M_FP = M_F_LO;
		for(int j = 2;j<=h; j++)
		{
			temp_M_FP = temp_M_FP + (temp_M_FP*T);
		}
		*M_FP = temp_M_FP; 
		//*M_FP = M_F_LO*(pow(T, L-Y) - 1)/(T-1);
	}
}

void getM_BF(double* M_BF, double M_F, double M_FP)
{
	const double MARGIN = 2;
	*M_BF = 0;
	//printf("\nbefore  M_F:%f, M_BF:%f, M_FP:%f, have to allocate:%f", M_F/(1024*1024*1024), (*M_BF)/(1024*1024*1024), M_FP/(1024*1024*1024), (M_F -M_FP)/(1024*1024*1024));
	if(M_F - M_FP > 0)
		*M_BF = M_F - M_FP - MARGIN;
	else
		*M_BF = 0.0;
}

int validateFilterMemoryLevels(double M_F, double M_F_HI, double M_F_LO)
{
	if(M_F_LO > M_F_HI) 
		{
			printf("\nagain");
			return 0;
		}

		return 1;
}

void set_M_B(double* M_F, double* M_B, double M, double M_F_HI, double M_F_LO)
{
	if(*M_F > M_F_HI) 
		*M_F = M_F_HI;
	if(*M_F < M_F_LO)
		*M_F = M_F_LO;
	if(M - (*M_F) <= (B*E))
		*M_F = M - (B*E);
	*M_B = (M - *M_F);
}

void set_M_F(double* M_F, double* M_B, double M, double M_F_HI, double M_F_LO)
{
	*M_F = M - *M_B;
	//if(*M_F > M_F_HI) 
	//	*M_F = M_F_HI;
	if(*M_F < M_F_LO)
		*M_F = M_F_LO;
}


void getM_F_LO(double* M_F_LO, double *M_B, double M, int T, double data)
{
	if((data)/(B) < (*M_B)*T/(B*E))
	{
		*M_F_LO = (data/(B))*F;
	}
	else
		*M_F_LO = (double)((*M_B)*(F)*T)/((B)*(E));
	/*if(*M_B + *M_F_LO > M)
	{
		*M_B = M/(1 + ((double)F*T)/(B*E));
		*M_F_LO = (double)((*M_B)*(F)*T)/((B)*(E));
	}*/
}

