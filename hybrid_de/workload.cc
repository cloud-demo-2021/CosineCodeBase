// compile with g++ -O3 -o workload workload.cc 

#include <iostream>
#include <vector>
#include <set>
#include <map>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <random>
#include <limits>
#include <assert.h>     /* assert */
#include <iostream>
#include <iomanip>
#include <map>
#include <random>
#include <fstream>
#include <sys/stat.h>
#include <sys/types.h>
#include <string>
#include <unordered_map>
#include <iterator>
#include <algorithm>
#include <chrono>
#include <set>
#include <sstream>
#include <string.h> 

using namespace std;


unsigned long long min_U = -2147483648;
unsigned long long max_U = 2147483647;
unsigned long long N = 10000000;
//unsigned long long U = pow(10, 10);
FILE *fp_bulk = fopen("src/bulkwrite.txt", "w");
FILE *fp_wl = fopen("src/workload.txt", "w");
std::vector<unsigned long long> keys;
std::vector<unsigned long long> values;
//std::vector<uint64_t> keys_special;
//std::vector<uint64_t> values_special;
//std::vector<uint64_t> keys_normal;
//std::vector<uint64_t> values_normal;
unsigned long long no_of_puts = 10000000;
unsigned long long no_of_gets = 100000;
unsigned long long no_of_ranges = 0;
unsigned long long range_length = 8000000;

unsigned long long U1 = 100000;
unsigned long long U2 = 100000000;
double pput = 0.5;
double pget = 0.5;

std::vector<unsigned long long>* special_keys_vec = new std::vector<unsigned long long>; 
std::vector<unsigned long long>* normal_keys_vec = new std::vector<unsigned long long>;
std::set<unsigned long long>* special_keys_set = new std::set<unsigned long long>; 
std::set<unsigned long long>* normal_keys_set = new std::set<unsigned long long>;

std::default_random_engine unif_generator(20);
std::default_random_engine bern_generator(10);


void generateKeysUniform(bool write_to_file, bool print) {
    std::random_device rd;  //Will be used to obtain a seed for the random number engine
    std::mt19937_64 gen(rd()); //Standard mersenne_twister_engine seeded with rd()
    uniform_int_distribution<unsigned long long> uni_dist1(min_U, max_U);
    uniform_int_distribution<unsigned long long> uni_dist2(1, 10000);
    keys.reserve(N);
    values.reserve(N);
    //char str[MAX_LENGTH];
    while (keys.size() < N) {
        unsigned long long key = uni_dist1(gen);
        unsigned long long val = uni_dist2(gen);
        if (print) {
            std::cout << key << ", " << val << std::endl;
        }
        keys.push_back(key);
        values.push_back(val);
        if(write_to_file)
        {
        	fprintf (fp_bulk, "b %d %d\n", (int)key, (int)val);
        }
    }
    //ssort(keys.begin(), keys.end());
}

void generatePutUniform(bool print) {
	std::random_device rd;  //Will be used to obtain a seed for the random number engine
    std::mt19937_64 gen(rd()); //Standard mersenne_twister_engine seeded with rd()
    uniform_int_distribution<unsigned long long> uni_dist1(min_U, max_U);
    uniform_int_distribution<unsigned long long> uni_dist2(1, 10000);
    for(int i=1;i<=no_of_puts;i++)
    {
    	unsigned long long key = uni_dist1(gen);
        unsigned long long val = uni_dist2(gen);
        if (print) {
            std::cout << key << ", " << val << std::endl;
        }
       	fprintf (fp_wl, "p %d %d\n", (int)key, (int)val);
    }
}

void generateGetUniform(bool print) {
	std::random_device rd;  //Will be used to obtain a seed for the random number engine
    std::mt19937_64 gen(rd()); //Standard mersenne_twister_engine seeded with rd()
    uniform_int_distribution<unsigned long long> uni_dist1(0, keys.size()-1);
    for(int i = 1;i<=no_of_gets;i++)
    {	
    	unsigned long long index = uni_dist1(gen);
    	unsigned long long key = keys[index];
        fprintf (fp_wl, "g %d\n", (int)key);	
    }
}

void generateRangeUniform(bool print) {
	std::random_device rd;  //Will be used to obtain a seed for the random number engine
    std::mt19937_64 gen(rd()); //Standard mersenne_twister_engine seeded with rd()
    uniform_int_distribution<unsigned long long> uni_dist1(0, keys.size()-1);
    sort(keys.begin(), keys.end()); 
    for(int i = 1;i<=no_of_ranges;i++)
    {	
    	unsigned long long index = uni_dist1(gen);
    	unsigned long long key1 = keys[index];
    	unsigned long long key2; 
    	//key2 = key1 + range_length;
    	if(index + range_length > keys.size()-1)
    	{
    		key2 = keys[keys.size()-1];
    	}
    	else
    	{
    		key2 = keys[index + range_length];
    	}
        fprintf (fp_wl, "r %d %d\n", (int)key1, (int)key2);	
    }
}



void get_skew_bulk(int64_t number_queries, double pget, bool print, bool write_to_file) {

	unsigned long long* skew_lookups = new unsigned long long[number_queries];
	int special_size = special_keys_vec->size();
	int normal_size = normal_keys_vec->size();
	int spl_key_count = 0, reg_key_count = 0;
	std::uniform_int_distribution<unsigned long long> unif_special_distribution(0, special_size-1);
	std::uniform_int_distribution<unsigned long long> unif_normal_distribution(0, normal_size-1);
	std::binomial_distribution<unsigned long long> bern_distribution_pget(1, pget);
	std::vector<unsigned long long>& special_vecRef = *(special_keys_vec);
	std::vector<unsigned long long>& normal_vecRef = *(normal_keys_vec);
	for (int64_t i = 0; i < number_queries; i++) {
		int type = bern_distribution_pget(bern_generator);
		unsigned long long key = 0;
		if (type == 1 && special_size >= 1) {
			unsigned long long index = unif_special_distribution(unif_generator);
			key = special_vecRef[index];
			spl_key_count++;
		}
		else {
			unsigned long long index = unif_normal_distribution(unif_generator);
			key = normal_vecRef[index];
			reg_key_count++;
		}
		skew_lookups[i] = key;
		if(print)
		{
			std::cout << i << ". " << key << std::endl;
		}
		if(write_to_file)
        {
        	fprintf (fp_wl, "g %llu\n",key);
        }
	}
	printf("special keys: %d, regular keys: %d\n", spl_key_count, reg_key_count);
}

void fill_skew_bulk_scattered_optimized(unsigned long long number_queries, unsigned long long U1, unsigned long long U2, double pput, double pget, bool print, bool write_to_file) {
	unsigned long long* dist_keys = new unsigned long long[number_queries];

	int64_t scale_factor = (int64_t) ((double)U2 / (double)U1);
	scale_factor += 1;
	if ((int64_t)U2 % (int64_t)U1 != 0) {
		scale_factor += 1;
	}

	std::uniform_int_distribution<unsigned long long> unif_special_distribution(1,U1);
	std::uniform_int_distribution<unsigned long long> unif_normal_distribution(1,1+U2);
	std::binomial_distribution<unsigned long long> bern_distribution_pput(1,pput);

	std::pair<std::set<unsigned long long>::iterator,bool> ret;


	for (int64_t i = 0; i < number_queries; i++) {
		unsigned long long key = 0;
		int type = bern_distribution_pput(bern_generator);
		if (type == 0) {
			int64_t unif_draw = (int64_t) unif_normal_distribution(unif_generator);
			int position = unif_draw / (scale_factor - 1); 
			key = (unsigned long long) (1 + position * scale_factor + (unif_draw % (scale_factor -1) )); 
			ret = normal_keys_set->insert(key);
			if (ret.second == true) {
				normal_keys_vec->push_back(key);
			}
		}
		else {
			key = (unsigned long long) (scale_factor * ((int) unif_special_distribution(unif_generator)));
			ret = special_keys_set->insert(key);
			if (ret.second == true) {
				special_keys_vec->push_back(key);
			}
		}
		dist_keys[i] = key;
		if(print)
		{
			std::cout << i << ". " << key << ", " << key << std::endl;
		}
		if(write_to_file)
        {
        	fprintf (fp_bulk, "b %llu %llu\n",key, key);
        }
	}
}

void fill_skew_put_scattered_optimized(unsigned long long number_queries, unsigned long long U1, unsigned long long U2, double pput, double pget, bool print, bool write_to_file) {
	unsigned long long* dist_keys = new unsigned long long[number_queries];

	int64_t scale_factor = (int64_t) ((double)U2 / (double)U1);
	scale_factor += 1;
	if ((int64_t)U2 % (int64_t)U1 != 0) {
		scale_factor += 1;
	}

	std::uniform_int_distribution<unsigned long long> unif_special_distribution(1,U1);
	std::uniform_int_distribution<unsigned long long> unif_normal_distribution(1,1+U2);
	std::binomial_distribution<unsigned long long> bern_distribution_pput(1,pput);

	std::pair<std::set<unsigned long long>::iterator,bool> ret;


	for (int64_t i = 0; i < number_queries; i++) {
		unsigned long long key = 0;
		int type = bern_distribution_pput(bern_generator);
		if (type == 0) {
			int64_t unif_draw = (int64_t) unif_normal_distribution(unif_generator);
			int position = unif_draw / (scale_factor - 1); 
			key = (unsigned long long) (1 + position * scale_factor + (unif_draw % (scale_factor -1) )); 
			ret = normal_keys_set->insert(key);
			if (ret.second == true) {
				normal_keys_vec->push_back(key);
			}
		}
		else {
			key = (unsigned long long) (scale_factor * ((int) unif_special_distribution(unif_generator)));
			ret = special_keys_set->insert(key);
			if (ret.second == true) {
				special_keys_vec->push_back(key);
			}
		}
		dist_keys[i] = key;
		if(print)
		{
			std::cout << i << ". " << key << ", " << key << std::endl;
		}
		if(write_to_file)
        {
        	fprintf (fp_wl, "p %llu %llu\n",key, key);
        }
	}
}

void generateKeysSkew()
{
	//unsigned long long num_lookups = U1;
	//unsigned long long num_inserts = N;
	fill_skew_bulk_scattered_optimized(N, U1, U2, pput, pget, false, true); 
	get_skew_bulk(no_of_gets, pget, false, true); 
	fill_skew_put_scattered_optimized(no_of_puts, U1, U2, pput, pget, false, true); 
}

int main(int argc, char* argv[])
{
	if(argc > 2)
	{
		fprintf(stderr, "Usage:<workload_type>, <N>, <#gets>, <#puts>, <max_U or U_1,U_2,p_put,p_get>\n");
		// ./workload 0 1000000 100000 100000 10000000000
		// ./workload 1 1000000 10000 10000 100000 100000000 0.3 0.1
		N = atol(argv[2]); 
		no_of_gets = atol(argv[3]); 
		no_of_puts = atol(argv[4]); 
		if(strcmp(argv[1], "0") == 0)
		{
			max_U = atol(argv[5]); 
		}
		else
		{
			U1 = atol(argv[5]); 
			U2 = atol(argv[6]); 
			pput = atof(argv[7]); 
			pget = atof(argv[8]); 
		}
	}
	if(argc <= 1 || strcmp(argv[1], "0") == 0)
	{
		printf("UNIFORM\n");
		generateKeysUniform(true, false); 
		generateGetUniform(false);
		generatePutUniform(false);
		generateRangeUniform(false);
	}
	else
	{
		printf("SKEW\n");
		generateKeysSkew(); 
	}
	fclose (fp_wl);
	fclose (fp_bulk);
	//out.close();
	return 0;
}