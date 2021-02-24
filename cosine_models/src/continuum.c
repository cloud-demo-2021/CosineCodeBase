#include <string.h>
#include <stdbool.h>

#include "include/continuum.h"
#include "include/environment_variable.h"
#include "include/VM_library.h"
#include "include/workload.h"
#include "include/index.h"
#include "include/design_space_navigation.h"
#include "include/compression_library.h"
#include "include/SLA_factors.h"

continuum_node* newContinuumNode(int provider_id, int* VM_arr, double mem_sum, int compression_id)
{
	continuum_node* node = (continuum_node*)malloc(sizeof(continuum_node));
	node->provider_id = provider_id;
	node->VM_arr = (int*)malloc(VM_libraries[provider_id].no_of_instances*sizeof(int));
	node->no_of_instances = 0;
	for(int i = 0;i<VM_libraries[provider_id].no_of_instances;i++)
	{
		node->VM_arr[i] = VM_arr[i];
		node->no_of_instances += VM_arr[i];
	}
	node->VM_design_arr = (continuum_design**)malloc(VM_libraries[provider_id].no_of_instances*sizeof(continuum_design*));
	node->mem_sum = mem_sum;
	node->compression_id = compression_id;
	node->next = NULL;
	node->down = NULL;
	return node;
}

void printContinuumNode(continuum_node* node)
{
	printf("\n********* Provider: %s ********* \n", VM_libraries[node->provider_id].provider_name);
	printf("********* VM instances ********* \n");
	for(int i = 0;i<VM_libraries[node->provider_id].no_of_instances;i++)
	{
		printf("%d instances of %s\n", node->VM_arr[i], VM_libraries[node->provider_id].name_of_instance[i]);
		if(node->VM_design_arr[i] != NULL)
		{
			printf("#queries: %.0lf, #data entries: %.0lf\n", node->VM_design_arr[i]->workload_VM, node->VM_design_arr[i]->data_VM);
			printf("Suggested design: T:%d K:%d Z:%d L:%d Y:%d M_B:%f M_BC:%f M_BF:%f M_FP:%f read_IO:%f update_IO:%f compression: %d\n", node->VM_design_arr[i]->T, node->VM_design_arr[i]->K, node->VM_design_arr[i]->Z, node->VM_design_arr[i]->L, node->VM_design_arr[i]->Y, node->VM_design_arr[i]->M_B/(1024*1024*1024), node->VM_design_arr[i]->M_BC/(1024*1024*1024), node->VM_design_arr[i]->M_BF/(1024*1024*1024), node->VM_design_arr[i]->M_FP/(1024*1024*1024), node->VM_design_arr[i]->read_cost, node->VM_design_arr[i]->update_cost, node->VM_design_arr[i]->compression_id);
		}
	}	
	printf("Memory of the configuration: %f GB\n", node->mem_sum/(1024*1024*1024));
	printf("Storage cost: $%f/month, compute cost: $%f/month, SLA cost: $%f/month, total cost of the configuration: $%f/month\n", node->storage_cost, (node->cost - node->storage_cost), node->SLA_cost, node->cost);
	printf("Total IOs: %f\n", node->total_IO);
	printf("Latency: %f days\n", node->latency/(3600*24));
}

void addContinuumNode(continuum_node* node)
{
	no_of_continuum_node++;
	if(p_continuum == NULL) // empty continuum
	{
		p_continuum = node;
	}
	else
	{
		if(node->cost <= p_continuum->cost) // insert at beggining
		{
			node->next = p_continuum;
			p_continuum = node;
		}
		else
		{
			continuum_node* head = p_continuum, *previous=NULL;
			while((head!=NULL) && (head->cost < node->cost))
			{
				previous = head;
				head = head->next;
			}
			if(head == NULL) // insert at end
			{
				previous->next = node;
			}
			else // insert at middle
			{
				node->next = head;
				previous->next = node;
			}
		}
	}
}

void updateContinuumNode(continuum_node* node_A, continuum_node* node_B) // update node A to node B
{
	node_A->provider_id = node_B->provider_id;
	node_A->VM_arr = node_B->VM_arr;
	node_A->VM_design_arr = node_B->VM_design_arr;
	node_A->mem_sum = node_B->mem_sum;
	node_A->total_IO = node_B->total_IO;
	node_A->storage_cost = node_B->storage_cost;
	node_A->no_of_instances = node_B->no_of_instances;
	node_A->latency = node_B->latency;
	node_A->compression_id = node_B->compression_id;
	node_A->SLA_cost = node_B->SLA_cost;
}

continuum_node* findBest(continuum_node* node)
{
	continuum_node* best = node;
	if(node->down && best->latency > node->down->latency)
	{
		best = node->down;
	}
	if(node->down && node->down->down && best->latency > node->down->down->latency)
	{
		best = node->down->down;
	}
	return best;
}

void correctContinuumForEachProvider()
{
	continuum_node* head = p_continuum, *best_AWS=NULL, *best_GCP=NULL, *best_AZURE=NULL, *temp, *current; 
	if(p_continuum == NULL)
	{
		printf("p_continuum is NULL\n");
		exit(0);
	}
	if(p_continuum->provider_id == 0)
	{
		best_AWS = p_continuum;
	}
	else if(p_continuum->provider_id == 1)
	{
		best_GCP = p_continuum;
	}
	else
	{
		best_AZURE = p_continuum;
	}
	while(head)
	{
		if(head->provider_id == 0)
		{
			if(best_AWS == NULL)
			{
				best_AWS = head;
			}
			else if(head->latency > best_AWS->latency)
			{
				updateContinuumNode(head, best_AWS);
			}
			else
			{
				best_AWS = head;
			}
		}
		else if(head->provider_id == 1)
		{
			if(best_GCP == NULL)
			{
				best_GCP = head;
			}
			else if(head->latency > best_GCP->latency)
			{
				updateContinuumNode(head, best_GCP);
			}
			else
			{
				best_GCP = head;
			}
		}
		else
		{
			if(best_AZURE == NULL)
			{
				best_AZURE = head;
			}
			else if(head->latency > best_AZURE->latency)
			{
				updateContinuumNode(head, best_AZURE);
			}
			else
			{
				best_AZURE = head;
			}
		}
		temp = head;
		head = head->next;
	}

	// Remove redundant configurations
	temp = p_continuum, head = p_continuum->next;
	while(head)
	{
		if(temp->cost == head->cost)
		{
			temp->next = head->next;
			head = temp->next;
		}
		else
		{
			temp = head;
			head = head->next;
		}
	}
}

void correctContinuum()
{
	correctContinuumForEachProvider(); 
	continuum_node* head = p_continuum, *best_AWS=NULL, *best_GCP=NULL, *best_AZURE=NULL, *node, *temp;
	while(head)
	{
		temp = head;
		if(head->provider_id == 0) //AWS
		{
			if(!best_AWS)
			{
				best_AWS = (continuum_node*)malloc(sizeof(continuum_node));
				updateContinuumNode(best_AWS, head);
			}
			else if(head->latency < best_AWS->latency)
			{
				updateContinuumNode(best_AWS, head);
			}
			if(best_GCP)
			{
				node = (continuum_node*)malloc(sizeof(continuum_node));
				updateContinuumNode(node, best_GCP);
				node->cost = head->cost;
				node->next = NULL;
				node->down = NULL;
				head->down = node;
				temp = head->down;
			}
			if(best_AZURE)
			{
				node = (continuum_node*)malloc(sizeof(continuum_node));
				updateContinuumNode(node, best_AZURE);
				node->cost = head->cost;
				node->next = NULL;
				node->down = NULL;
				temp->down = node;
			}
		}
		if(head->provider_id == 1) //GCP
		{
			if(!best_GCP)
			{
				best_GCP = (continuum_node*)malloc(sizeof(continuum_node));
				updateContinuumNode(best_GCP, head);
			}
			else if(head->latency < best_GCP->latency)
			{
				updateContinuumNode(best_GCP, head);
			}
			if(best_AWS)
			{
				node = (continuum_node*)malloc(sizeof(continuum_node));
				updateContinuumNode(node, best_AWS);
				node->cost = head->cost;
				node->next = NULL;
				node->down = NULL;
				head->down = node;
				temp = head->down;
			}
			if(best_AZURE)
			{
				node = (continuum_node*)malloc(sizeof(continuum_node));
				updateContinuumNode(node, best_AZURE);
				node->cost = head->cost;
				node->next = NULL;
				node->down = NULL;
				temp->down = node;
			}
		}
		if(head->provider_id == 2) //AZURE
		{
			if(!best_AZURE)
			{
				best_AZURE = (continuum_node*)malloc(sizeof(continuum_node));
				updateContinuumNode(best_AZURE, head);
			}
			else if(head->latency < best_AZURE->latency)
			{
				updateContinuumNode(best_AZURE, head);
			}
			if(best_AWS)
			{
				node = (continuum_node*)malloc(sizeof(continuum_node));
				updateContinuumNode(node, best_AWS);
				node->cost = head->cost;
				node->next = NULL;
				node->down = NULL;
				head->down = node;
				temp = head->down;
			}
			if(best_GCP)
			{
				node = (continuum_node*)malloc(sizeof(continuum_node));
				updateContinuumNode(node, best_GCP);
				node->cost = head->cost;
				node->next = NULL;
				node->down = NULL;
				temp->down = node;
			}
		}
		head = head->next;
	}
}

void printContinuumAtGap()
{
	double gap = 1000.0;
	continuum_node* head = p_continuum, *temp, *previous;
	double cost = 1000.0;
	int provider_id = 0;
	bool found = false;

	while(head)
	{
		temp = head;
		while(temp)
		{
			if(temp->provider_id == provider_id)
			{
				found = true;
				break;
			}
			temp = temp->down;
		}
		if(found == true)
		{
			while(cost <= temp->cost)
			{
				printf("%f\t\n", cost);
				cost += gap;
			}
			break;
		}
		previous = temp;
		head = head->next;
	}
	while(head)
	{
		//printf("Cost: %f\n", head->cost);
		temp = head;
		while(temp && temp->provider_id != provider_id)
		{
			temp = temp->down;
		}
		if(temp->cost > cost)
		{
			printf("%f\t%f\n", cost, previous->latency/(3600));	
			cost += gap;	
		}
		previous = temp;
		head = head->next;
	}	
}
void printContinuum(bool entire)
{
	continuum_node* head = p_continuum, *temp, *best, *overall_best = p_continuum;
	while(head)
	{
		//printf("%f\t%f\t%f\t%d\t%s\n", head->cost, head->latency/(3600*24), head->mem_sum/(1024*1024*1024), head->no_of_instances, VM_libraries[head->provider_id].provider_name);
		//printf("\n");
		if(entire)
		{
			temp = head;
			while(temp)
			{
				//printContinuumNode(temp);
				printf("%f\t%f (SLA)\t%f\t%f\t%s\t%s\n", temp->cost, temp->SLA_cost, temp->latency/(3600), temp->mem_sum/(1024*1024*1024), VM_libraries[temp->provider_id].provider_name, compression_libraries[temp->compression_id].compression_name);
				temp=temp->down;
			}
			printf("\n");
		}
		else
		{
			best = findBest(head);
			if(overall_best->latency < best->latency)
			{
				best = overall_best;
			}
			else
			{
				overall_best = best;
			}
			printf("%f\t%f (SLA)\t%f\t%s\t%s\t", best->cost, best->SLA_cost, best->latency/(3600), VM_libraries[best->provider_id].provider_name, compression_libraries[best->compression_id].compression_name);
			//printContinuumNode(best);
			for(int i = 0;i<VM_libraries[best->provider_id].no_of_instances;i++)
			{
				if(best->VM_design_arr[i] != NULL)
				{
					printf("%s\n", best->VM_design_arr[i]->msg);
				}
			}
		}
		head = head->next;
	}
}

void buildContinuum()
{
	printf("Into buildContinuum()\n");
	no_of_continuum_node = 0;	
	//for(int compression_id = 0; compression_id<no_of_compression_library;compression_id++)
	for(int compression_id = 0; compression_id<1;compression_id++)
	{ 
		for(int i = 0; i < no_of_cloud_providers; i++)
		//for(int i = 0; i < 1; i++)
		{
			pricing_scheme = VM_libraries[i].provider_name;
			/* Fixing the block size, storage cost, and IOPS for each provider */
			long double storage = ((double)(N)/(1024*1024*1024))*E;
			//printf("storage reduced from %Lf size to ", storage);
			if(using_compression == 1)
			{
				storage = storage*(1.0 - compression_libraries[compression_id].space_reduction_ratio);
				printf("%Lf with %s compression for %s\n", storage, compression_libraries[compression_id].compression_name, pricing_scheme);
			}
			double monthly_storage_cost, SLA_cost = 0.0;
			double MBps;
			if(i == 0) // AWS
			{
				MBps = 3500;
				B = 256*1024/(E);
				IOPS = MBps*pow(10,6)/(B*E);
				if(IOPS > 15000) 
				{
					IOPS = 15000;
				}
				if(storage > 75)
				{
					monthly_storage_cost = (storage-75)*0.1; // $0.1 per GB-month https://aws.amazon.com/ebs/pricing/
				}
				else
				{
					monthly_storage_cost = storage*0.1; // $0.1 per GB-month https://aws.amazon.com/ebs/pricing/
				}
			}
			else if(i == 1) // GCP
			{
				MBps = read_percentage*720/100 + write_percentage*160/100; // taking average
				B = 16*1024/(E);
				IOPS = MBps*pow(10,6)/(B*E);
				if(IOPS > 30000) 
				{
					IOPS = 30000;
				}
				monthly_storage_cost = storage*0.17;
			}
			else // AZURE
			{
				B = 8*1024/(E);
				if(storage <= 32)
				{
					IOPS = 120;
					monthly_storage_cost = 5.28;
				}
				else if(storage > 32 && storage <= 64)
				{
					IOPS = 240;
					monthly_storage_cost = 10.21;
				}
				else if(storage > 64 && storage <= 128)
				{
					IOPS = 500;
					monthly_storage_cost = 19.71;
				}
				else if(storage > 128 && storage <= 256)
				{
					IOPS = 1100;
					monthly_storage_cost = 38.02;
				}
				else if(storage > 256 && storage <= 512)
				{
					IOPS = 2300;
					monthly_storage_cost = 73.22;
				}
				else if(storage > 512 && storage <= 2000)
				{
					IOPS = 5000;
					monthly_storage_cost = 135.17;
				}
				else
				{
					IOPS = 7500;
					monthly_storage_cost = 259.05;
				}
			}
			if(using_compression == 1)
			{
				//printf("B chnaged from %d entries to ", B);
				B = B/(1.0 - compression_libraries[compression_id].space_reduction_ratio);
				//printf("%d entries with %s compression for %s\n", B, compression_libraries[compression_id].compression_name, pricing_scheme);
			}
			if(enable_SLA == 1)
			{
				computeSLARelatedCost(i, &SLA_cost);
			}

			/* Iterating over all possible VM combinations */
			char* filename = (char*)malloc(200*sizeof(char));
			strcpy(filename, VM_libraries[i].provider_name);
			strcat(filename, "_VM_library.txt");
			ssize_t read;
			size_t len = 0;
			char* line = NULL;
			double workload_proportion;
			FILE *fp = fopen(filename, "r");
			int* VM_arr = (int*)malloc(VM_libraries[i].no_of_instances*sizeof(int));
			//printf("***************** %s *************** \n", VM_libraries[i].provider);
			int node_sum = 0;
			double mem_sum = 0;
			int config_id = 1;
			while ((read = getline(&line, &len, fp)) != -1) 
			{	
				int index = 0;
		        char* pch, *ptr;
		        pch = strtok (line," ,.-");
		        while (pch != NULL)
		        {
		        	VM_arr[index++] = strtol(pch, &ptr, 10);
		        	pch = strtok (NULL, " ,.-");
		        }
		        mem_sum = 0, node_sum = 0;
		        for(int j = 0;j<VM_libraries[i].no_of_instances;j++)
		        {
		        	//printf("%d ", VM_arr[j]);
		        	node_sum += VM_arr[j];
		        	mem_sum += VM_arr[j]*VM_libraries[i].mem_of_instance[j];
		        }
		        evaluateConfiguration(i, VM_arr, mem_sum*1024*1024*1024, monthly_storage_cost, SLA_cost, compression_id);
		        //printf("Done for configuration %d (%s)\n\n", config_id++, pricing_scheme);
	   		}
	   		//fclose(fp);
			//printf("Done for %s\n", pricing_scheme);
		}
	}
}


void evaluateConfiguration(int provider_id, int* VM_arr, double mem_sum, double monthly_storage_cost, double SLA_cost, int compression_id)
{
	double workload_VM, mem_VM, data_VM;
	continuum_node* newnode = newContinuumNode(provider_id, VM_arr, mem_sum, compression_id);
	newnode->total_IO = 0;
	newnode->storage_cost = monthly_storage_cost;
	newnode->SLA_cost = SLA_cost;
	newnode->cost = monthly_storage_cost + SLA_cost; 
	double dev_ops_cost = 0.0;
	int VM_type;
	for(int i = 0;i<VM_libraries[provider_id].no_of_instances;i++)
	{
	    if(VM_arr[i] != 0)
	    {
	    	VM_type = i;
	        workload_VM = VM_libraries[provider_id].mem_of_instance[i]*query_count*(1024*1024*1024)/mem_sum;
	        mem_VM = VM_libraries[provider_id].mem_of_instance[i]*(1024*1024*1024);
	        data_VM = VM_libraries[provider_id].mem_of_instance[i]*(N)*(1024*1024*1024)/mem_sum;
	        newnode->cost += VM_arr[i]*monthlyPrice(VM_libraries[provider_id].rate_of_instance[i]);
	        if(enable_SLA == 1 && enable_dev_ops) // for AWS and Azure
			{
				if(provider_id == 0) // AWS
				{
					dev_ops_cost = dev_ops_AWS * VM_arr[i];
				}
				else if(provider_id == 1) // GCP
				{
					if(i == 0)
					{
						dev_ops_cost = 0.1184 * VM_arr[i];
					}
					else if(i == 1)
					{
						dev_ops_cost = 0.2368 * VM_arr[i];
					}
					else if(i == 2)
					{
						dev_ops_cost = 0.4736 * VM_arr[i];	
					}
					else if(i == 3)
					{
						dev_ops_cost = 0.9472 * VM_arr[i];
					}
					else if(i == 4)
					{
						dev_ops_cost = 1.8944 * VM_arr[i];
					}
					else if(i == 5)
					{
						dev_ops_cost = 3.7888 * VM_arr[i];
					}
					else if(i == 6)
					{
						dev_ops_cost = 5.6832 * VM_arr[i];
					}
				}
				else // Azure
				{
					dev_ops_cost = dev_ops_Azure;
				}
				newnode->SLA_cost += dev_ops_cost;
				newnode->cost += dev_ops_cost; 
			}
	        if(FitsInMemory(mem_VM, data_VM))
	        {
	        	newnode->VM_design_arr[i] = NULL;
	        	newnode->total_IO = 0;
	        	continue;
	        }
	        else
	        {
	        	newnode->VM_design_arr[i] = (continuum_design*)malloc(sizeof(continuum_design));
	        	newnode->VM_design_arr[i]->data_VM = data_VM;
	        	newnode->VM_design_arr[i]->workload_VM = workload_VM;
	        	if(!existing_system)
	        	{
	        		navigateDesignSpaceForContinuumSingleMachine(mem_VM, data_VM, workload_VM, &newnode->VM_design_arr[i], compression_id);
	        	}
	        	else
	        	{
	        		if(getCostForExistingSystems(mem_VM, data_VM, workload_VM, &newnode->VM_design_arr[i], newnode->cost, provider_id, compression_id) != 0)
	        		{
	        			continue;
	        		}
	        	}
	        }
	        newnode->total_IO += newnode->VM_design_arr[i]->total_cost;
	    }
    }
    newnode->latency = newnode->total_IO/IOPS;
    /* TAKE INTO ACCOUNT CPU EFFECTS */
    if(using_compression == 1)
    {
    	newnode->latency = (newnode->VM_design_arr[VM_type]->read_cost*query_count*read_percentage/IOPS)*(100.0+compression_libraries[compression_id].get_overhead)/100
    					   + (newnode->VM_design_arr[VM_type]->update_cost*query_count*write_percentage/IOPS)*(100.0+compression_libraries[compression_id].put_overhead)/100;
    }


    if(existing_system)
    {
    	//printf("%f\t%f\n", newnode->cost, newnode->total_IO);
    	if(newnode->total_IO > 0.0000001)
    		addContinuumNode(newnode);
    }
    else
    {
    	addContinuumNode(newnode);	
    }
    //printContinuumNode(newnode);
}




