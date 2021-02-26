extern crate prometheus;
extern crate log;
extern crate env_logger;

use hybrid_de::lib_helper;
use hybrid_de::lib_test;
use hybrid_de::lib_workload;
//use std::fs;
use prometheus::{TextEncoder, Encoder};
use log::{info};
use threadpool::ThreadPool;
use hybrid_de::configuration::CONFIGURATION;

fn main() 
{
    //let pool = ThreadPool::new(CONFIGURATION.CPUS);
    env_logger::init();
    //lib_test::test_small_workload();
    // for i in 10..11 {
    //     for j in 0..4 {
    //         for k in 0..7600 {
    //             let filename = format!("{}.{}.{}.{}", i, j, 0, k);
    //             debug!("{}", &filename);
    //             fs::remove_file(&filename).unwrap();
    //         }
    //     }
    // }
    
    //lib_test::test_regular_workload();
    //lib_test::test_update_heavy_workload();
    //lib_test::test_order();
    //lib_test::run_all_bench();

    //debug!("\t\tPlease wait ... Generating workload!");
    let mut workload_file: std::string::String = "workload.txt".to_string();
    let mut bulkwrite_file: std::string::String = "bulkwrite.txt".to_string();
    //lib_workload::set_workload_specifications(true, true, true, 1_000_000, 100_000, 100_000, i32::min_value(), i32::max_value(), &mut workload_file, &mut bulkwrite_file);
    lib_helper::run_with_time(&bulkwrite_file, &workload_file);
    //lib_helper::run(&bulkwrite_file, &workload_file);
    
    // //output metrics
    // let mut buffer = Vec::new();
    // let encoder = TextEncoder::new();

    // // Gather the metrics.
    // let metric_families = prometheus::gather();
    // // Encode them to send.
    // encoder.encode(&metric_families, &mut buffer).unwrap();

    // let output = String::from_utf8(buffer.clone()).unwrap();
    // println!("{}", output);
}
