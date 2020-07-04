#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "include/VM_library.h"
#include "include/environment_variable.h"

void initializeVMLibraries()
{
	VM_libraries = (VM_library*)malloc(no_of_cloud_providers*sizeof(VM_library));

	/* ********************************** initialize VMs of AWS *********************************  */
	VM_libraries[0].provider_name = "AWS";
	VM_libraries[0].no_of_instances = 6;
	VM_libraries[0].name_of_instance = (char**)malloc(VM_libraries[0].no_of_instances*sizeof(char*));
	VM_libraries[0].mem_of_instance = (double*)malloc(VM_libraries[0].no_of_instances*sizeof(double));
	VM_libraries[0].rate_of_instance = (double*)malloc(VM_libraries[0].no_of_instances*sizeof(double));

	VM_libraries[0].name_of_instance[0] = "r5d.large";
	VM_libraries[0].name_of_instance[1] = "r5d.xlarge";
	VM_libraries[0].name_of_instance[2] = "r5d.2xlarge";
	VM_libraries[0].name_of_instance[3] = "r5d.4xlarge";
	VM_libraries[0].name_of_instance[4] = "r5d.12xlarge";
	VM_libraries[0].name_of_instance[5] = "r5d.24xlarge";

	VM_libraries[0].mem_of_instance[0] = 16;
	VM_libraries[0].mem_of_instance[1] = 32;
	VM_libraries[0].mem_of_instance[2] = 64;
	VM_libraries[0].mem_of_instance[3] = 128;
	VM_libraries[0].mem_of_instance[4] = 384;
	VM_libraries[0].mem_of_instance[5] = 768;

	VM_libraries[0].rate_of_instance[0] = 0.091;
	VM_libraries[0].rate_of_instance[1] = 0.182;
	VM_libraries[0].rate_of_instance[2] = 0.364;
	VM_libraries[0].rate_of_instance[3] = 0.727;
	VM_libraries[0].rate_of_instance[4] = 2.181;
	VM_libraries[0].rate_of_instance[5] = 4.362;

	/* ********************************** initialize VMs of GCP *********************************  */
	VM_libraries[1].provider_name = "GCP";
	VM_libraries[1].no_of_instances = 7;
	VM_libraries[1].name_of_instance = (char**)malloc(VM_libraries[1].no_of_instances*sizeof(char*));
	VM_libraries[1].mem_of_instance = (double*)malloc(VM_libraries[1].no_of_instances*sizeof(double));
	VM_libraries[1].rate_of_instance = (double*)malloc(VM_libraries[1].no_of_instances*sizeof(double));

	VM_libraries[1].name_of_instance[0] = "n1-highmem-2";
	VM_libraries[1].name_of_instance[1] = "n1-highmem-4";
	VM_libraries[1].name_of_instance[2] = "n1-highmem-8";
	VM_libraries[1].name_of_instance[3] = "n1-highmem-16";
	VM_libraries[1].name_of_instance[4] = "n1-highmem-32";
	VM_libraries[1].name_of_instance[5] = "n1-highmem-64";
	VM_libraries[1].name_of_instance[6] = "n1-highmem-96";

	VM_libraries[1].mem_of_instance[0] = 13;
	VM_libraries[1].mem_of_instance[1] = 26;
	VM_libraries[1].mem_of_instance[2] = 52;
	VM_libraries[1].mem_of_instance[3] = 104;
	VM_libraries[1].mem_of_instance[4] = 208;
	VM_libraries[1].mem_of_instance[5] = 416;
	VM_libraries[1].mem_of_instance[6] = 624;

	VM_libraries[1].rate_of_instance[0] = 0.0745;
	VM_libraries[1].rate_of_instance[1] = 0.1491;
	VM_libraries[1].rate_of_instance[2] = 0.2981;
	VM_libraries[1].rate_of_instance[3] = 0.5962;
	VM_libraries[1].rate_of_instance[4] = 1.1924;
	VM_libraries[1].rate_of_instance[5] = 2.3849;
	VM_libraries[1].rate_of_instance[6] = 3.5773;

	/* ********************************** initialize VMs of AZURE *********************************  */
	VM_libraries[2].provider_name = "AZURE";
	VM_libraries[2].no_of_instances = 7;
	VM_libraries[2].name_of_instance = (char**)malloc(VM_libraries[2].no_of_instances*sizeof(char*));
	VM_libraries[2].mem_of_instance = (double*)malloc(VM_libraries[2].no_of_instances*sizeof(double));
	VM_libraries[2].rate_of_instance = (double*)malloc(VM_libraries[2].no_of_instances*sizeof(double));

	VM_libraries[2].name_of_instance[0] = "E2 v3";
	VM_libraries[2].name_of_instance[1] = "E4 v3";
	VM_libraries[2].name_of_instance[2] = "E8 v3";
	VM_libraries[2].name_of_instance[3] = "E16 v3";
	VM_libraries[2].name_of_instance[4] = "E20 v3";
	VM_libraries[2].name_of_instance[5] = "E32 v3";
	VM_libraries[2].name_of_instance[6] = "E64 v3";

	VM_libraries[2].mem_of_instance[0] = 16;
	VM_libraries[2].mem_of_instance[1] = 32;
	VM_libraries[2].mem_of_instance[2] = 64;
	VM_libraries[2].mem_of_instance[3] = 128;
	VM_libraries[2].mem_of_instance[4] = 160;
	VM_libraries[2].mem_of_instance[5] = 256;
	VM_libraries[2].mem_of_instance[6] = 512;

	VM_libraries[2].rate_of_instance[0] = 0.0782;
	VM_libraries[2].rate_of_instance[1] = 0.1564;
	VM_libraries[2].rate_of_instance[2] = 0.3128;
	VM_libraries[2].rate_of_instance[3] = 0.6256;
	VM_libraries[2].rate_of_instance[4] = 0.7409;
	VM_libraries[2].rate_of_instance[5] = 1.2512;
	VM_libraries[2].rate_of_instance[6] = 2.5024;

	printVMLibraries();
}

void printVMLibraries()
{
	for(int i = 0; i < no_of_cloud_providers; i++)
	{
		printf("\n\n");
		printf("*********************** CLOUD PROVIDER: %s ***********************  \n", VM_libraries[i].provider_name);
		for(int j = 0; j < VM_libraries[i].no_of_instances; j++)
		{
			printf("VM %s\t %f GB\t$%f/hour\n", VM_libraries[i].name_of_instance[j], VM_libraries[i].mem_of_instance[j], VM_libraries[i].rate_of_instance[j]);
		}
		printf("\n");
	}
}

void getAllVMCombinations()
{
	int* soln = (int*)malloc(VM_libraries[0].no_of_instances*sizeof(int));
	for(int i = 0; i < no_of_cloud_providers; i++)
	{
		char* filename = (char*)malloc(200*sizeof(char));
		strcpy(filename, VM_libraries[i].provider_name);
		strcat(filename, "_VM_library.txt");
		FILE *fp = fopen(filename, "w");
		fclose(fp);
		printf("Getting combinations for %s.\n", VM_libraries[i].provider_name);
		for(int j = 1; j <= machines; j++)
		{
			//getCombinations(i, 0, VM_libraries[i].no_of_instances-1, j, soln);
			getCombinationsHomegenous(i, 0, VM_libraries[i].no_of_instances-1, j, soln);
		}
	}
}

void printCombinations(int* soln, int provider_index)
{
	char* filename = (char*)malloc(200*sizeof(char));
	strcpy(filename, VM_libraries[provider_index].provider_name);
	strcat(filename, "_VM_library.txt");
	FILE *fp = fopen(filename, "a+");
	for(int i=0;i<VM_libraries[provider_index].no_of_instances;i++)
	{
		//printf("%d ", soln[i]);
		fprintf(fp, "%d ", soln[i]);
	}
	fprintf(fp, "\n");
	//printf("\n");
	fclose(fp);
}

void getCombinationsHomegenous(int provider_index, int start, int end, int mc, int* soln)
{
	for(int i = start;i<=end;i++)
	{
		soln[i] = mc;
		printCombinations(soln, provider_index);
		soln[i] = 0;
	}
}

void getCombinations(int provider_index, int start, int end, int mc, int* soln)
{
	if(mc == 1)
	{
		for(int i = start; i <= end; i++)
		{
			soln[i] += 1;
			printCombinations(soln, provider_index);
			soln[i] -= 1;
		}
	}
	else
	{
		soln[start] += 1;
		getCombinations(provider_index, start, end, mc-1, soln);
		soln[start] -= 1;
		if(start != end) 
		{
			getCombinations(provider_index, start+1, end, mc, soln);
		}
	}
}







