#ifndef ENVIRONMENT_VARIABLE_GUARD
#define ENVIRONMENT_VARIABLE_GUARD

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include <stdbool.h> 

//******************************* ENVIRONMENTAL PARAMETERS ***************************
#define MIN_BUDGET 7000
#define MAX_BUDGET 8000
#define RECORD_SIZE 128 //byte, RocksDB
#define AVG_CREDIT_SIZE 6*1024 // byte, 6KB
#define AVG_CREDIT_COST 0.000444 // $
#define PER_IO_COST (AVG_CREDIT_COST/AVG_CREDIT_SIZE)*DISK_BLOCK_SIZE
#define PER_GB_STORAGE_COST 0.285 //$ per GB per month
#define MAX_STORAGE_SIZE 500 // GB
#define B_TREE_CACHE_DISCOUNT_FACTOR 0.1 // B-Tree cache discounting factorr (set empirically)

#define E 128 // entry size in byte
//#define B 4*1024/E // 4KB is default block size
#define F 64 // key size in byte
//#define N 1000000000000
long int N; // total #entries in dataset
bool wl_var_set;
int MIN_RAM_SIZE;
double RAM_BLOCK_COST;
double IOPS; // IOs allowed per sec (changes with cloud provider)
int B; // block size in terms of #entries residing in a block (changes with cloud provider)
char* pricing_scheme;
double network_bandwidth;
char* existing_system; // WT or rocks or FASTER or FASTER_H
#define PRINT(x) ('x') 

//******************************* End of ENVIRONMENTAL PARAMETERS ***************************



//******************************* COMPRESSION SCHEME ***************************************
enum compression_scheme{
	ZLIB = 2, SNAPPY = 1, NONE = 0
};
enum compression_scheme compression_used;
//******************************* End of COMPRESSION SCHEME ***************************************



//******************************* OPTIMIZATIONS/TUNINGS ***************************************
static int using_compression = 0;
static int enable_SLA = 0;
static char scenario = 'A'; // 'W' for worst-case, 'A' for avg-case
static char execution_mode = 'C'; //'C' for continuum, 'N' for normal, 'P' for command line arguments
static int machines = 30;
static bool enable_performance_window = false;
static bool enable_parallelism = true;
static bool enable_CLL = false;
static bool enable_Rosetta = true;
//******************************* End of OPTIMIZATIONS/TUNINGS ***************************************

#endif
