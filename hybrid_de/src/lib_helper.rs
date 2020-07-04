extern crate prometheus;
use parking_lot::deadlock;
use std::thread;
use std::time::Duration;
use is_sorted::IsSorted;
use log::{info, error, debug};
use std::convert::TryInto;
use std::io::{BufReader, Error};
use std::io::prelude::*;
use std::fs::File;
use std::time::{Instant};
use std::sync::{Arc};
use atomic_counter::{AtomicCounter};
//use std::collections::HashMap;
use threadpool::ThreadPool;
use super::configuration::CONFIGURATION;
use super::lib_template::{Record};
//use super::lib_workload;
//use super::lib_in_memory;
use super::lib_lsm_tree::LSMTree;
use super::lib_on_disk::lib_disk_level::DiskLevel;
use super::lib_on_disk::lib_disk_run::Run;

use prometheus::{TextEncoder, Encoder};
use crate::metrics::{GET_IO_COUNTER, PUT_IO_COUNTER};

pub fn parse_instruction(mut _instruction: String) -> (String, i32, String)
{
	let mut _key = 0;
	let mut _value = "";
	let op_code = &_instruction[0..2];
	if op_code.trim() == "b" || op_code.trim() == "p" || op_code.trim() == "r"
	{
		let slice = &_instruction[2..];
		let index: usize = match slice.find(' ')
		{
			Some(v) => v,
			None => 100, // arbitrary random value
		};
		//debug!("{}, space at {:#?}", slice, index);
		if index != 100
		{
			_key = (&slice[0..index]).parse::<i32>().unwrap();
			_value = &slice[index+1..];
			//let record = lib_template::Record{key: _key, value: _value};
		}
		else 
		{
			debug!("ERROR!!");
		}
	}
	else 
	{
		_key = (&_instruction[2..]).parse::<i32>().unwrap();
		//debug!("{}", _key);
	}
	(op_code.to_string(), _key, _value.to_string())
}

// checks:
// 1. level size = sum of run sizes
// 2. file is sorted, file fence pointers are correct, run fence pointers correspond to file fence pointers, both fence pointers are sorted
// prints: size of each level + run
pub fn check_tree_metadata(lsm_tree: &LSMTree) {
	let buffer = lsm_tree.buffer.read();
	println!("BUFFER SIZE {}: CAPACITY {}: FULLNESS {}", buffer.size(), buffer.capacity(), buffer.size() as f64 / buffer.capacity() as f64);
	drop(buffer);
	let levels = lsm_tree.levels.read();
	for level in levels.iter() {
		println!("LEVEL {}: SIZE {}: CAPACITY {}: FULLNESS {}", level.level, level.size(), level.capacity(), level.size() as f64 / level.capacity() as f64);
		let mut run_size = 0;
		let runs = level.runs.read();
        for run in runs.iter() {
			println!("RUN SIZE {}: CAPACITY {}: FULLNESS {}", run.size, run.capacity, run.size as f64 / run.capacity as f64);
			run_size += run.size;
			for i in 0..run.files.len() {
				let file = &run.files[i];
				let file_records = file.read_all_file_records();
				assert!(IsSorted::is_sorted(&mut file_records.iter()));
				assert!(file.fence_pointers[0] == run.fence_pointers[i]);
				for j in (0..file_records.len()).step_by(CONFIGURATION.BLOCK_SIZE / CONFIGURATION.RECORD_SIZE) {
					assert!(file_records[j].key == file.fence_pointers[j / (CONFIGURATION.BLOCK_SIZE / CONFIGURATION.RECORD_SIZE)]);
				}
				assert!(IsSorted::is_sorted(&mut file.fence_pointers.iter()));
			}
			assert!(IsSorted::is_sorted(&mut run.fence_pointers.iter()));
        }
        assert!(run_size == level.size());
	}
}

pub fn run(bulkwrite_file: &String, workload_file: &String) {
	let mut lsm_tree = LSMTree::create_lsmtree();
	bulkwrite(bulkwrite_file, &mut lsm_tree);
	run_file(workload_file, lsm_tree);
}

pub fn run_with_time(bulkwrite_file: &String, workload_file: &String) {
	let mut lsm_tree: LSMTree = LSMTree::create_lsmtree();
	bulkwrite(bulkwrite_file, &mut lsm_tree);
	//run_file_for_benchmark(bulkwrite_file, &mut lsm_tree);
	let start = Instant::now();
	run_file_for_benchmark(workload_file, lsm_tree);
	let duration = start.elapsed();
	println!("Time elapsed is: {:?}", duration);

	// output metrics
    let mut buffer = Vec::new();
    let encoder = TextEncoder::new();

    // Gather the metrics.
    let metric_families = prometheus::gather();
    // Encode them to send.
    encoder.encode(&metric_families, &mut buffer).unwrap();

    let output = String::from_utf8(buffer.clone()).unwrap();
	println!("{}", output);
}

// Useful for checking behavior of LSM Tree
pub fn run_file(_workload_file: &String, lsm_tree: LSMTree) {
	let pool = ThreadPool::new(CONFIGURATION.CPUS);
	let wl = File::open(_workload_file).expect("Error in opening workload file!");
	let wl = BufReader::new(wl);
	let lsm_tree = Arc::new(lsm_tree);
	let mut _num_misses = 0;
	//debug!("LSM tree at beginning of running workload file has {} levels", lsm_tree.levels.len());

    for line in wl.lines() {
		let lsm_tree = lsm_tree.clone();
		let (op_code, key, value) = parse_instruction(line.unwrap());

		pool.execute(move || {
			//debug!("{} {} {}", op_code, key, value);
			match op_code.trim() {
				"b" | "p" => lsm_tree.put(&key, &value),
				"r" => {
					let range = lsm_tree.find_range(&key, &value.parse::<i32>().unwrap());
					println!("{:?}", range);
				}
				"g" => {
					let mut val = 0;
					match lsm_tree.get(&key) {
						Some(val) => {
							print!("{}\n", val);
						}, 
						None =>  {
							_num_misses += 1;
							println!("");
						}
					}
				}
				"d" => lsm_tree.delete(&key),
				_ => error!("ERROR, BAD OPCODE")
			}
		})
	}
	pool.join();
	check_tree_metadata(&lsm_tree);
	lsm_tree.delete_files();
	//debug!("failed gets: {}", num_misses);
	//lsm_tree.print_stats();
}

// No prints for benchmarking
pub fn run_file_for_benchmark(_workload_file: &String, lsm_tree: LSMTree) {
	let pool = ThreadPool::new(CONFIGURATION.CPUS - CONFIGURATION.COMPACTION_THREADS);

	GET_IO_COUNTER.reset();
	PUT_IO_COUNTER.reset();
	//PC_LEVELS_COUNTER.reset();

	let wl = File::open(_workload_file).expect("Error in opening workload file!");
	let wl = BufReader::new(wl);
	let lsm_tree = Arc::new(lsm_tree);
	
	// Create a background thread which checks for deadlocks every 10s
	// thread::spawn(move || {
	// 	loop {
	// 		thread::sleep(Duration::from_secs(10));
	// 		let deadlocks = deadlock::check_deadlock();
	// 		if deadlocks.is_empty() {
	// 			continue;
	// 		}

	// 		println!("{} deadlocks detected", deadlocks.len());
	// 		for (i, threads) in deadlocks.iter().enumerate() {
	// 			println!("Deadlock #{}", i);
	// 			for t in threads {
	// 				println!("Thread Id {:#?}", t.thread_id());
	// 				println!("{:#?}", t.backtrace());
	// 			}
	// 		}
	// 	}
	// });

	let mut max_time = 0;
    for line in wl.lines() {
		let lsm_tree = lsm_tree.clone();
		let (op_code, key, value) = parse_instruction(line.unwrap());

		let start = Instant::now();
		pool.execute(move || {
			match op_code.trim() {
				"b" | "p" => lsm_tree.put(&key, &value),
				"r" => {
					let _range = lsm_tree.find_range(&key, &value.parse::<i32>().unwrap());
				}
				"g" => {
					let mut val = 0;
					lsm_tree.get(&key);
				}
				"d" => lsm_tree.delete(&key),
				_ => error!("ERROR, BAD OPCODE")
			}
		});
		let duration = start.elapsed();
		max_time = std::cmp::max(max_time, duration.as_micros());
	}
	pool.join();
	println!("tail latency is {}", max_time);
	//check_tree_metadata(&lsm_tree);
	lsm_tree.delete_files();
}

fn create_record_from_line(line: Result<String, Error>) -> Record {
	let (op_code, key, value) = parse_instruction(line.unwrap().to_string());
	assert!(op_code.trim() == "b");
	Record::create_record(key, value)
}

pub fn bulkwrite(bulkwrite_file: &String, lsm_tree: &mut LSMTree) {
	let br = File::open(bulkwrite_file).expect("Error in opening bulkwrite file!");
	let br = BufReader::new(br);
	let v = &mut br.lines().map(|l| create_record_from_line(l)).collect::<Vec<_>>();
	v.reverse();
	let num_records = v.len();
	let mut records_read = 0;
	let last_level: usize = ((num_records * CONFIGURATION.RECORD_SIZE * (CONFIGURATION.SIZE_RATIO - 1)) as f64 / (CONFIGURATION.BUFFER_CAPACITY * CONFIGURATION.SIZE_RATIO) as f64).ceil().log(CONFIGURATION.SIZE_RATIO as f64).ceil() as usize;
	debug!("last level is {}", last_level);
	let mut level: usize = 0;
	let mut run = 0;
	let mut levels = lsm_tree.levels.write();
	while records_read < num_records {
		if run == 0 {
			level += 1;
			let mut run_capacity = CONFIGURATION.BUFFER_CAPACITY * CONFIGURATION.SIZE_RATIO.pow(level as u32) / CONFIGURATION.RUNS_PER_LEVEL;
			levels.push(DiskLevel::empty_level(run_capacity, level))
		}
		let mut run_capacity = CONFIGURATION.BUFFER_CAPACITY * CONFIGURATION.SIZE_RATIO.pow(level as u32) / CONFIGURATION.RUNS_PER_LEVEL;
		let max_run_records = if level == last_level {
			((run_capacity as f64 * (CONFIGURATION.RUNS_PER_LEVEL as f64 / CONFIGURATION.RUNS_LAST_LEVEL as f64)) as usize) / CONFIGURATION.RECORD_SIZE
		} else {
			run_capacity / CONFIGURATION.RECORD_SIZE
		};
		let records_to_read = std::cmp::min(num_records - records_read, max_run_records);
		let current = &mut v[records_read..records_read + records_to_read].to_vec();
		current.sort_by(|a, b| a.key.cmp(&b.key));
		current.dedup_by(|a, b| a.key == b.key);
		debug!("creating run {} at level {} with size {}", CONFIGURATION.RUNS_PER_LEVEL - 1 - run, level, current.len() * CONFIGURATION.RECORD_SIZE);
		let mut runs = levels[level - 1].runs.write();
		runs.insert(0, Run::create_run(current.len() * CONFIGURATION.RECORD_SIZE, run_capacity, &records_to_bytes(current), level, run));
		levels[level - 1].run_counter.inc();
		debug!("run counter at level {} is {}", level, levels[level - 1].run_counter.get());
		levels[level - 1].add_size(current.len() * CONFIGURATION.RECORD_SIZE);
		run = (run + 1) % CONFIGURATION.RUNS_PER_LEVEL;
		records_read += records_to_read;
	}
	assert!(records_read == num_records);
}

pub fn generate_filename(level: usize, run: usize, file_idx: usize) -> String {
	level.to_string() + "." + &run.to_string() + "." + &file_idx.to_string()
}

pub fn bytes_to_records(bytes: &[u8]) -> Vec<Record> {
	let mut records: Vec<Record> = Vec::new();
	for i in (0..bytes.len()).step_by(CONFIGURATION.RECORD_SIZE) {
		let boundary = i + CONFIGURATION.KEY_SIZE;
		let key = i32::from_be_bytes(bytes[i..boundary].try_into().unwrap());
		let value = String::from_utf8(bytes[boundary..i + CONFIGURATION.RECORD_SIZE].to_vec()).unwrap();
		records.push(Record::create_record(key, value));
	}
	records
}

pub fn records_to_bytes(records: &Vec<Record>) -> Vec<u8> {
	let mut bytes = Vec::new();
	for record in records.iter() {
		bytes.extend(&record.key.to_be_bytes());
		bytes.extend(record.value.as_bytes());
	}
	bytes
}

pub fn pad_string(to_pad: &str, target_len: usize) -> String {
	let mut result = to_pad.to_string();
	while result.len() < target_len {
		result += " ";
	}
	result
}

pub fn binary_search_fp(fence_pointers: &Vec<i32>, key: &i32) -> Option<usize> {
	// If not found, binary search will return error with index it can be inserted in
	let fp_idx = fence_pointers.binary_search(&key);
	let fp_idx = match fp_idx {
		Err(other_idx) => {
			if other_idx == 0 {
				return None
			} 
			other_idx - 1
		}
		Ok(idx) => idx, 
	};
	Some(fp_idx)
}
