#ifndef COMPRESSION_LIBRARY
#define COMPRESSION_LIBRARY

typedef struct compression_library
{
	char* compression_name;
	double get_overhead;
	double put_overhead;
	double space_reduction_ratio;
}compression_library;
compression_library* compression_libraries;
static int no_of_compression_library = 3;

void initializeCompressionLibraries();
void printCompressionLibraries();


#endif
