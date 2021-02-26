#ifndef PARALLELISM
#define PARALLELISM

static double prop_of_parallelizable_code_reads_Cosine = 0.9647; 
static double prop_of_parallelizable_code_writes_Cosine = 0.797; 

static double prop_of_parallelizable_code_reads_Cosine_LSH = 0.97; 
static double prop_of_parallelizable_code_writes_Cosine_LSH = 0.9; 

static double prop_of_parallelizable_code_reads_rocks = 0.9609; 
static double prop_of_parallelizable_code_writes_rocks = 0.8017; 

static double prop_of_parallelizable_code_reads_FASTER = 0.9922; 
static double prop_of_parallelizable_code_writes_FASTER = 0.9761; 

static double prop_of_parallelizable_code_reads_WT = 0.94; 
static double prop_of_parallelizable_code_writes_WT = 0.25; 

double overall_prop_parallelizable_code;

void setOverallProportionOfParallelizableCode();
void setDesignSpecificOverallProportionOfParallelizableCode(char* design_class);


#endif
