#ifndef ENVIRONMENT_VARIABLE_GUARD
#define ENVIRONMENT_VARIABLE_GUARD

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include <stdbool.h> 

// ****************************** MACROs ******************************
//#define NO_RAM_BLOCKS 8
//#define RAM_BLOCKS_BASE 1.55
#define MIN_BUDGET 7000
#define MAX_BUDGET 8000
#define RECORD_SIZE 128 //byte, RocksDB
//#define DISK_BLOCK_SIZE 512 // byte
#define AVG_CREDIT_SIZE 6*1024 // byte, 6KB
#define AVG_CREDIT_COST 0.000444 // $
#define PER_IO_COST (AVG_CREDIT_COST/AVG_CREDIT_SIZE)*DISK_BLOCK_SIZE
#define PER_GB_STORAGE_COST 0.285 //$ per GB per month]
#define MAX_STORAGE_SIZE 500 // GB

//******************************* ENVIRONMENTAL PARAMETERS ***************************
#define E 128 // B
//#define B 4*1024/E // 4KB is default block size
#define F 64 // B
//#define N 1000000000000
long int N;
//#define D 1
#define PRINT(x) ('x') 

//******************************* End of ENVIRONMENTAL PARAMETERS ***************************

//******************************* COMPRESSION SCHEME ***************************************
enum compression_scheme{
	ZLIB = 2, SNAPPY = 1, NONE = 0
};
//******************************* COMPRESSION SCHEME ***************************************

enum compression_scheme compression_used;
static int using_compression = 0;
static int enable_SLA = 0;
bool wl_var_set;
int MIN_RAM_SIZE;
double RAM_BLOCK_COST;
double IOPS;
int B;
//int E = 128;
char* pricing_scheme;
static char scenario = 'A'; // 'W' for worst-case, 'A' for avg-case
static char execution_mode = 'C'; //'C' for continuum, 'N' for normal, 'P' for command line arguments
double network_bandwidth;
static int machines = 30;
char* existing_system; // WT or rocks or FASTER
static bool enable_performance_window = false;

#endif
