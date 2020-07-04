#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "include/compression_library.h"
#include "include/environment_variable.h"

void initializeCompressionLibraries()
{
	compression_libraries = (compression_library*)malloc(no_of_compression_library*sizeof(compression_library));

 	/**************************************************** NO COMPRESSION  ****************************************************/
	compression_libraries[0].compression_name = "NONE";
	compression_libraries[0].get_overhead = 1;
	compression_libraries[0].put_overhead = 1;
	compression_libraries[0].space_reduction_ratio = 0.0;

	/**************************************************** SNAPPY  ****************************************************/
	compression_libraries[1].compression_name = "SNAPPY";
	compression_libraries[1].get_overhead = 0.53;
	compression_libraries[1].put_overhead = 8.21;
	compression_libraries[1].space_reduction_ratio = 0.68;

	/**************************************************** ZLIB  ****************************************************/
	compression_libraries[2].compression_name = "ZLIB";
	compression_libraries[2].get_overhead = 25.45;
	compression_libraries[2].put_overhead = 31.26;
	compression_libraries[2].space_reduction_ratio = 0.83;

	printCompressionLibraries();
}

void printCompressionLibraries()
{
	for(int i = 0; i<no_of_compression_library;i++)
	{
		printf("Compression type: %s\n", compression_libraries[i].compression_name);
		printf("get overhead: %f, put overhead: %f", compression_libraries[i].get_overhead, compression_libraries[i].put_overhead);
		printf(" space_reduction_ratio: %f\n", compression_libraries[i].space_reduction_ratio);
		printf("\n\n");
	}
}