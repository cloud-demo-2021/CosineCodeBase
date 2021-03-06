# README #

## Installation
Follow [these instructions](https://doc.rust-lang.org/book/ch01-01-installation.html#installation) to install Rust.

## Running
To compile and run, use `cargo run`

## Configuration
- To change the configuration (i.e. how much memory is allocated to the buffer, the file size, the bloom filter false positive rate, etc.), go to [src/configuration.rs](https://bitbucket.org/chatterjeesubarna/hybrid_de/src/master/src/configuration.rs).
- From [src/configuration.rs](https://bitbucket.org/chatterjeesubarna/hybrid_de/src/master/src/configuration.rs), you can also change the size ratio (T) and the runs per level (K). Set T = K for a tiered design and K = 1 for a leveled design. Any other values of T, K such that T > K is considered a hybrid design.
- Another option for changing T and K is through the command line: `cargo run -- T K`, i.e. `cargo run -- 4 1`, if you want a leveled LSM tree with size ratio 4. 
	- See [benchmark.sh](https://bitbucket.org/chatterjeesubarna/hybrid_de/src/master/benchmark.sh) for a simple shell script that allows you to try many different values of T and K.

## Tests and Benchmarking
- The entry point of the LSM tree is in [src/main.rs](https://bitbucket.org/chatterjeesubarna/hybrid_de/src/master/src/main.rs). From here, you can determine which tests/benchmarks you’d like to run.
	- Note: adding any print lines will cause the tests to assert fail, since it compares the stdout of the program with the expected output generated by [evaluate.py](https://bitbucket.org/chatterjeesubarna/hybrid_de/src/master/evaluate.py). Even if the test assert fails, you can manually check `diff output.txt expected.txt` to ensure the only differences are the print statements. 
- To see the current tests/benchmarks, go to [src/lib_test/mod.rs](https://bitbucket.org/chatterjeesubarna/hybrid_de/src/master/src/lib_test/mod.rs). You can add your own benchmark here or change the existing benchmarks to run different workloads. The `set_workload_specifications` function is what creates the bulkwrites and workload, and writes them into the specified files. To see the function definition, go to [src/lib_workload/mod.rs](https://bitbucket.org/chatterjeesubarna/hybrid_de/src/master/src/lib_workload/mod.rs).
    - Sometimes, you might want to run multiple configurations on the exact same workload. In those cases, I would recommend running the benchmarks on one configuration, and then commenting out the `set_workload_specifications` call in `run_bench` when running the benchmarks on other configurations.
