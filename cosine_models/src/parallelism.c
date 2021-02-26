#include <string.h>

#include "include/parallelism.h"
#include "include/workload.h"
#include "include/environment_variable.h"


void setOverallProportionOfParallelizableCode()
{
	if(enable_parallelism)
	{
		if(existing_system)
		{
			if(strcmp(existing_system, "rocks") == 0) // RocksDB
			{
				overall_prop_parallelizable_code = ((double)(read_percentage + rmw_percentage  + long_scan_percentage + long_scan_empty_percentage)*prop_of_parallelizable_code_reads_rocks/100) + ((double)(write_percentage + blind_update_percentage)*prop_of_parallelizable_code_writes_rocks/100);
			}
			else if(strstr(existing_system, "FASTER") != NULL) // Any FASTER versions
			{
				overall_prop_parallelizable_code = ((double)(read_percentage + rmw_percentage + blind_update_percentage + long_scan_percentage + long_scan_empty_percentage)*prop_of_parallelizable_code_reads_FASTER/100) + ((double)write_percentage*prop_of_parallelizable_code_writes_FASTER/100);
			}
			else if(strcmp(existing_system, "WT") == 0) // WiredTiger
			{
				overall_prop_parallelizable_code = ((double)(read_percentage + rmw_percentage + blind_update_percentage + long_scan_percentage + long_scan_empty_percentage)*prop_of_parallelizable_code_reads_WT/100) + ((double)write_percentage*prop_of_parallelizable_code_writes_WT/100);
			}
		}
		else
		{
			overall_prop_parallelizable_code = ((double)(read_percentage + rmw_percentage  + long_scan_percentage + long_scan_empty_percentage)*prop_of_parallelizable_code_reads_Cosine/100) + ((double)write_percentage*prop_of_parallelizable_code_writes_Cosine/100);
		}
		printf("%f proportion of code is parallelizable.\n\n", overall_prop_parallelizable_code);
	}
	else
	{
		overall_prop_parallelizable_code = 1.0;	
	}
}

void setDesignSpecificOverallProportionOfParallelizableCode(char* design_class)
{
	if(enable_parallelism)
	{
		if(strcmp(design_class, "LSH") == 0)
		{
			overall_prop_parallelizable_code = ((double)(read_percentage + rmw_percentage + blind_update_percentage + long_scan_percentage + long_scan_empty_percentage)*prop_of_parallelizable_code_reads_Cosine_LSH/100) + ((double)write_percentage*prop_of_parallelizable_code_writes_Cosine_LSH/100);
			// if(workload_type == 1) // for skew we're doing this else FASTER's speedup will dominate results
			// {
			// 	overall_prop_parallelizable_code = prop_of_parallelizable_code_reads_FASTER;
			// }
		}
		else
		{
			overall_prop_parallelizable_code = ((double)(read_percentage + rmw_percentage + blind_update_percentage + long_scan_percentage + long_scan_empty_percentage)*prop_of_parallelizable_code_reads_Cosine/100) + ((double)write_percentage*prop_of_parallelizable_code_writes_Cosine/100);
		}
		//printf("%f proportion of code is parallelizable.\n\n", overall_prop_parallelizable_code);
	}
	else
	{
		overall_prop_parallelizable_code = 1.0;	
	}
}