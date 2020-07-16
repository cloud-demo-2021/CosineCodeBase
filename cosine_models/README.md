# README #

## Compiling
In order to compile, you'll need to pre-install `make` and `gcc`. Then run,  
`make clean`.  
`make`.    


## Running
Run with `./build/a.out`. The default is a uniform workload with 10<sup>12</sup> entries, 10<sup>10</sup> queries (20% reads and 80% writes) which can be changed within `CosineCodeBase/cosine_models/src/workload.c`.


## Interpretation of Output

The output is the cost-performance continuum comprising of a set of rows. Each row of the output contains 5 columns -   

 - budget ($)
 - part of the budget taken up by SLA. By default SLA is turned off. You can turn it on by setting `enable_SLA=1` in `CosineCodeBase/cosine_models/src/include/environment_variable.h`
 - latency (in days)
 - name of the cloud provider
 - class of the storage engine design
