#include "include/SLA_factors.h"
#include "include/environment_variable.h"

void initializeSLAFactors()
{
	/******************** DB Migration Cost ********************/

	DB_migration_cost_AWS = 0.115; // $/GB
	DB_migration_cost_GCP = 0.17; // $/GB
	DB_migration_cost_Azure = 0.17; // $/GB

	dev_ops_AWS = 0.02; // $/instance
	dev_ops_Azure = 6; // $6/month in the basic plan (https://azure.microsoft.com/en-us/pricing/details/devops/azure-devops-services/)
	// dev_ops_GCP will be fixed based on the VM type

	backup_AWS = 0.05; // $0.05 per GB-Month
	backup_GCP = 0.17; // $0.170 per GB/month for SSD storage capacity
	backup_Azure = 0.0448; // $0.0448 per GB

	/******************** Other Factors ********************/
}

void computeSLARelatedCost(int cloud_provider, double* SLA_cost)
{
	if(enable_DB_migration && enable_backup)
	{
		if(cloud_provider == 0) // AWS
		{
			*SLA_cost = (DB_migration_cost_AWS + backup_AWS)*(N*E)/(1024*1024*1024);
		}
		else if(cloud_provider == 1) // GCP
		{
			*SLA_cost = (DB_migration_cost_GCP + backup_GCP)*(N*E)/(1024*1024*1024);
		}
		else // Azure
		{
			*SLA_cost = (DB_migration_cost_Azure + backup_Azure)*(N*E)/(1024*1024*1024);
		}
	}
	else if(enable_DB_migration && !enable_backup)
	{
		if(cloud_provider == 0) // AWS
		{
			*SLA_cost = (DB_migration_cost_AWS)*(N*E)/(1024*1024*1024);
		}
		else if(cloud_provider == 1) // GCP
		{
			*SLA_cost = (DB_migration_cost_GCP)*(N*E)/(1024*1024*1024);
		}
		else // Azure
		{
			*SLA_cost = (DB_migration_cost_Azure)*(N*E)/(1024*1024*1024);
		}
	}
	else if(!enable_DB_migration && enable_backup)
	{
		if(cloud_provider == 0) // AWS
		{
			*SLA_cost = (backup_AWS)*(N*E)/(1024*1024*1024);
		}
		else if(cloud_provider == 1) // GCP
		{
			*SLA_cost = (backup_GCP)*(N*E)/(1024*1024*1024);
		}
		else // Azure
		{
			*SLA_cost = (backup_Azure)*(N*E)/(1024*1024*1024);
		}
	}
}