#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>

#include "../environment_variable.h"

int main()
{
	double ans;
	for(int t=8;t<=256;t=t*2)
	{
		printf("\nT=%d\n", t);
		for(int k=1;k<=t;k=k+(log(t)/log(2)))
		{
			ans = pow(k, 1.0/t);
			printf("\n%d\t%7f", k, ans);
		}
	}
	
	return 0;
}