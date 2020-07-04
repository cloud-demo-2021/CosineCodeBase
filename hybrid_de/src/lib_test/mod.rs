extern crate rand;
//use std::collections::{HashMap};
use std::process::{Command, Stdio};
use std::fs::{File};

use super::lib_workload;
use super::lib_helper;
use super::lib_lsm_tree::LSMTree;
//use super::configuration::CONFIGURATION;

//use bloom::{BloomFilter};
//use rand::Rng;
//use test::Bencher;
use gag::Redirect;
use log::{info, debug};
use threadpool::{ThreadPool};

// Run with cargo test -- --nocapture

#[cfg(test)]
#[test]
fn test_bloom_filter_with_fpr()
{
	let expected_num_items: usize = 10000;
	let false_positive_rate = 0.001;
	let mut filter = BloomFilter::with_rate(false_positive_rate,expected_num_items);

	let mut vector_existing_elements: Vec<i32> = Vec::new();
	let mut vector_non_existing_elements: Vec<i32> = Vec::new();
	let mut rng = rand::thread_rng();
	for i in 0..expected_num_items
	{
		let element: i32 = rng.gen();
		vector_existing_elements.push(element);
		filter.insert(&element);
	}	

	for i in 0..expected_num_items
	{
		let mut element: i32;
		loop 
		{
			element = rng.gen();
			if vector_existing_elements.contains(&element) == false
			{
				break;
			}
		}     		
		vector_non_existing_elements.push(element);
	}	

	for element in vector_existing_elements.iter() 
	{
		assert_eq!(filter.contains(&element), true);
	}

	let mut false_positives = 0;
	for element in vector_non_existing_elements.iter() 
	{
		if filter.contains(&element) == true
		{
			false_positives += 1;
		}
	}
	debug!("\n");
	debug!("\t\t{} false positives out of {} queries!", false_positives, (2*expected_num_items));
	debug!("\t\tFPR: {:.4}", false_positives as f32/(2.0*expected_num_items as f32));
	drop(filter);
}

#[test]
fn test_bloom_filter_with_bits_per_entry()
{
	let expected_num_items: usize = 10000;
	let num_bits = 5;
	let num_hashes = bloom::optimal_num_hashes(num_bits,expected_num_items);
	let mut filter = BloomFilter::with_size(num_bits,num_hashes);

	let mut vector_existing_elements: Vec<i32> = Vec::new();
	let mut vector_non_existing_elements: Vec<i32> = Vec::new();
	let mut rng = rand::thread_rng();
	for _i in 0..expected_num_items
	{
		let element: i32 = rng.gen();
		vector_existing_elements.push(element);
		filter.insert(&element);
	}	

	for _i in 0..expected_num_items
	{
		let mut element: i32;
		loop 
		{
			element = rng.gen();
			if vector_existing_elements.contains(&element) == false
			{
				break;
			}
		}     		
		vector_non_existing_elements.push(element);
	}	

	for element in vector_existing_elements.iter() 
	{
		assert_eq!(filter.contains(&element), true);
	}

	let mut false_positives = 0;
	for element in vector_non_existing_elements.iter() 
	{
		if filter.contains(&element) == true
		{
			false_positives += 1;
		}
	}
	
	debug!("\n");
	debug!("\t\t{} false positives out of {} queries!", false_positives, (2*expected_num_items));
	debug!("\t\tFPR: {:.4}", false_positives as f32/(2.0*expected_num_items as f32));
	drop(filter);
}

//#[test]
// can't be run from cargo test (times out)
pub fn test_lsm_tree(bulkwrite_file: &String, workload_file: &String) {
	let full_workload = File::create("full_workload.txt").expect("error creating file");
	let mut child = Command::new("cat")
					.args(&[bulkwrite_file, workload_file])
					.stdout(Stdio::from(full_workload))
					.spawn()
					.expect("failed to cat");
	
	let ecode = child.wait()
				.expect("failed to wait on child");

	assert!(ecode.success());
	//Open a log
	{ 
		let output = File::create("output.txt").expect("error creating file");
		let _redirect = Redirect::stdout(output).unwrap();
		lib_helper::run(bulkwrite_file, workload_file);
	}

	let expected = File::create("expected.txt").expect("error creating file");
	let mut child = Command::new("python")
					.args(&["evaluate.py", "full_workload.txt"])
					.stdout(Stdio::from(expected))
					.spawn()
					.expect("failed to evaluate");
	
	let ecode = child.wait()
				.expect("failed to wait on child");
	assert!(ecode.success());

	let output = Command::new("python")
				.args(&["check_diff_no_order.py", "expected.txt", "output.txt"])
				.output()
				.expect("failed to get diff");
	
	// let output = Command::new("diff")
	// 			.args(&["expected.txt", "output.txt"])
	// 			.output()
	// 			.expect("failed to get diff");

	debug!("Stdout is {:?}", output.stdout);
	debug!("Stderr is {:?}", output.stderr);
	debug!("{:?}", output.status);
	assert!(output.stdout.len() == 0);
	assert!(output.stderr.len() == 0);
}

pub fn test_small_workload() {
	let mut workload_file: std::string::String = "small_workload.txt".to_string();
	let mut bulkwrite_file: std::string::String = "small_bulkwrite.txt".to_string();
	//lib_workload::set_workload_specifications(true, true, true, 1_000_000, 100_000, 100_000, i32::min_value(), i32::max_value(), &mut workload_file, &mut bulkwrite_file);
	test_lsm_tree(&bulkwrite_file, &workload_file);
}

pub fn test_regular_workload() {
	let mut workload_file: std::string::String = "large_workload.txt".to_string();
	let mut bulkwrite_file: std::string::String = "large_bulkwrite.txt".to_string();
	// let mut workload_file: std::string::String = "regular_workload.txt".to_string();
	// let mut bulkwrite_file: std::string::String = "regular_bulkwrite.txt".to_string();
	//lib_workload::set_workload_specifications(true, true, true, 1_000_000, 100_000, 100_000, i32::min_value(), i32::max_value(), &mut workload_file, &mut bulkwrite_file);
	test_lsm_tree(&bulkwrite_file, &workload_file);
}

pub fn test_update_heavy_workload() {
	let mut workload_file: std::string::String = "update_workload.txt".to_string();
	let mut bulkwrite_file: std::string::String = "update_bulkwrite.txt".to_string();
	//lib_workload::set_workload_specifications(true, true, true, 1_000_000, 100_000, 100_000, -50_000, 50_000, &mut workload_file, &mut bulkwrite_file);
	test_lsm_tree(&bulkwrite_file, &workload_file);
}

#[test]
pub fn test_order() {
	let mut workload_file: std::string::String = "workload.txt".to_string();
	let keys = lib_workload::create_repeated_workload(1000000, &mut workload_file);
	let mut lsm_tree: LSMTree = LSMTree::create_lsmtree();
	lib_helper::run_file(&workload_file, &mut lsm_tree);
	let buffer = lsm_tree.buffer.read();
	let mut new_keys = buffer.buffer.clone();
	for level in &lsm_tree.levels {
		for run in level.runs.iter().rev() {
			let run_data = run.get_all_records();
			for record in run_data.iter() {
				match new_keys.get(&record.key) {
					Some(&value) => { debug!("newer: {}, older: {}", value, record.value); assert!(value > record.value) },
					None => (),
				};
				new_keys.insert(record.key, record.value);
			}
		}
	}
	drop(buffer);
	lsm_tree.delete_files();
	assert!(new_keys.len() == keys.len());
}


fn run_bench(_is_data_uniform: bool, _is_query_uniform: bool, _is_single_result: bool, _n_bulkwrites: usize, _n_puts: usize, _n_gets: usize, workload_file: &mut String, bulkwrite_file: &mut String) {
	//lib_workload::set_workload_specifications(_is_data_uniform, _is_query_uniform, _is_single_result, _n_bulkwrites, _n_puts, _n_gets, i32::min_value(), i32::max_value(), workload_file, bulkwrite_file);
	lib_helper::run_with_time(&bulkwrite_file, &workload_file);
}

pub fn bench_mixed() {
	let mut bulkwrite_file: std::string::String = "bulkwrite_mixed.txt".to_string();
	let mut workload_file: std::string::String = "workload_mixed.txt".to_string();
	run_bench(true, true, true, 1_000_000, 1_000_000, 1_000_000, &mut workload_file, &mut bulkwrite_file);
}

fn bench_read_only() {
	let mut bulkwrite_file: std::string::String = "bulkwrite_read_only.txt".to_string();
	let mut workload_file: std::string::String = "workload_read_only.txt".to_string();
	run_bench(true, true, true, 1_000_000, 0, 2_000_000, &mut workload_file, &mut bulkwrite_file);
}

fn bench_write_leaning() {
	let mut bulkwrite_file: std::string::String = "bulkwrite_write_leaning.txt".to_string();
	let mut workload_file: std::string::String = "workload_write_leaning.txt".to_string();
	run_bench(true, true, true, 1_000_000, 1_200_000, 800_000, &mut workload_file, &mut bulkwrite_file);
}

fn bench_write_leaning_v2() {
	let mut bulkwrite_file: std::string::String = "bulkwrite_write_leaning_v2.txt".to_string();
	let mut workload_file: std::string::String = "workload_write_leaning_v2.txt".to_string();
	run_bench(true, true, true, 1_000_000, 1_400_000, 600_000, &mut workload_file, &mut bulkwrite_file);
}

fn bench_read_heavy() {
	let mut bulkwrite_file: std::string::String = "bulkwrite_read_heavy.txt".to_string();
	let mut workload_file: std::string::String = "workload_read_heavy.txt".to_string();
	run_bench(true, true, true, 1_000_000, 400_000, 1_600_000, &mut workload_file, &mut bulkwrite_file);
}

fn bench_write_only() {
	let mut bulkwrite_file: std::string::String = "bulkwrite_write_only.txt".to_string();
	let mut workload_file: std::string::String = "workload_write_only.txt".to_string();
	run_bench(true, true, true, 1_000_000, 2_000_000, 0, &mut workload_file, &mut bulkwrite_file);
}

fn bench_write_heavy() {
	let mut bulkwrite_file: std::string::String = "bulkwrite_write_heavy.txt".to_string();
	let mut workload_file: std::string::String = "workload_write_heavy.txt".to_string();
	run_bench(true, true, true, 1_000_000, 1_600_000, 400_000, &mut workload_file, &mut bulkwrite_file);
}

pub fn run_all_bench() {
	// println!("----------MIXED-----------");
	// bench_mixed();
	// println!("----------READ-ONLY-----------");
	// bench_read_only();
	// println!("----------READ-HEAVY-----------");
	// bench_read_heavy();
	// println!("----------WRITE-ONLY-----------");
	// bench_write_only();
	// println!("----------WRITE-HEAVY-----------");
	// bench_write_heavy();
	println!("----------WRITE-LEANING-----------");
	bench_write_leaning_v2();
}

// // for now, the benchmarks take too long (run later)
// // to run this, install nightly with `rustup install nightly`, then `rustup default nightly`, then `cargo bench`
// fn setup_bench(_is_data_uniform: bool, _is_query_uniform: bool, _is_single_result: bool, _n_bulkwrites: usize, _n_puts: usize, _n_gets: usize) -> (LSMTree, String) {
// 	let mut workload_file: std::string::String = "workload.txt".to_string();
// 	let mut bulkwrite_file: std::string::String = "bulkwrites.txt".to_string();
// 	lib_workload::set_workload_specifications(_is_data_uniform, _is_query_uniform, _is_single_result, _n_bulkwrites, _n_puts, _n_gets, i32::min_value(), i32::max_value(), &mut workload_file, &mut bulkwrite_file);
// 	let mut lsm_tree = LSMTree::create_lsmtree();
// 	lib_helper::run_file_for_benchmark(&bulkwrite_file, &mut lsm_tree);
// 	(lsm_tree, workload_file)
// }

// #[bench]
// fn bench_mixed(b: &mut Bencher) {
// 	let (mut lsm_tree, workload_file) = setup_bench(true, true, true, 10_000_000, 1_000_000, 1_000_000);
// 	b.iter(|| lib_helper::run_file_for_benchmark(&workload_file, &mut lsm_tree));
// 	lsm_tree.delete_files();
// }

// #[bench]
// fn bench_read_only(b: &mut Bencher) {
// 	let (mut lsm_tree, workload_file) = setup_bench(true, true, true, 10_000_000, 0, 2_000_000);
// 	b.iter(|| lib_helper::run_file_for_benchmark(&workload_file, &mut lsm_tree));
// 	lsm_tree.delete_files();
// }




