#ifndef VM_LIBRARY
#define VM_LIBRARY

typedef struct VM_library
{
	char* provider_name;
	int no_of_instances;
	char** name_of_instance;
	double* mem_of_instance; // GB
	double* rate_of_instance; // hourly price

}VM_library;
VM_library* VM_libraries;
static int no_of_cloud_providers = 3;

void initializeVMLibraries();
void printVMLibraries();
void getCombinations(int provider_index, int start, int end, int mc, int* soln);
void printCombinations(int* soln, int provider_index);
void getAllVMCombinations();
void getCombinationsHomegenous(int provider_index, int start, int end, int mc, int* soln);

#endif
