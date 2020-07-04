extern crate rand;
use log::{error, debug};

use rand::Rng;
use rand::distributions::{Distribution as distribution, Uniform as uniform, Normal as normal};
use std::convert::TryInto;
use std::fs::File;
use std::collections::{HashMap};
use std::fs::OpenOptions;
use std::io::prelude::*;
use std::process::Command;

#[derive(Debug)]
pub enum Distribution 
{
	UNIFORM, 
	SKEW,
}

#[derive(Debug)]
#[allow(non_camel_case_types)]
pub enum LookupType 
{
	SINGLE_RESULT,
	NO_RESULT,
}

struct Workload
{
	// Distribution
	data_distribution: Distribution,
	query_distribution: Distribution,
	lookup_type: LookupType,

	// #Queries
	n_bulkwrites: usize,
	n_puts: usize,
	n_gets: usize,
	
	// Keys
	key_min_value: i32,
	key_max_value: i32,

	// Data
	workload_file: String,
	bulkwrite_file: String,
}

pub fn setup_file(file: &String) -> File {
	// remove if file already exists!
	Command::new("rm")
	.arg(&file)
	.output()
	.expect("ls command failed to start");
	
	// create new bulkwrite file 
	let  file = OpenOptions::new()
		.write(true)
		.create_new(true)
		.append(true)
		.open(&file)
		.unwrap();

	file
}

//_n_puts should be multiple of 20
pub fn create_repeated_workload(_n_puts: usize, _workload_file: &mut String) -> HashMap<i32, i32> {
	let mut workload_file = setup_file(_workload_file);
	let mut rng = rand::thread_rng();
	let mut keys = HashMap::new();
	for _i in 0.._n_puts {
		let key: i32 = rng.gen_range(0, _n_puts as i32 / 20);
		let value = match keys.get(&key) {
			Some(&value) => value,
			None => 0,
		};
		keys.insert(key, &value + 1);
		if let Err(e) = writeln!(workload_file, "p {} {}", key, value) {
			error!("FAIL {}", e);
		}
	}
	debug!("\t\tCreated Repeated Workload with {} distinct keys", keys.len());
	keys
}

pub fn set_workload_specifications(_is_data_uniform: bool, _is_query_uniform: bool, _is_single_result: bool, _n_bulkwrites: usize, _n_puts: usize, _n_gets: usize, _key_min_value: i32, _key_max_value: i32, _workload_file: &mut String, _bulkwrite_file: &mut String)
{
	let workload = Workload{data_distribution: match _is_data_uniform {true => Distribution::UNIFORM, false => Distribution::SKEW},
					query_distribution: match _is_query_uniform {true => Distribution::UNIFORM, false => Distribution::SKEW}, 
					lookup_type: match _is_single_result {true => LookupType::SINGLE_RESULT, false => LookupType::NO_RESULT},
					n_bulkwrites: _n_bulkwrites, 
					n_puts: _n_puts, 
					n_gets: _n_gets,
					key_min_value: _key_min_value,
					key_max_value: _key_max_value,
					workload_file: _workload_file.to_string(),
					bulkwrite_file: _bulkwrite_file.to_string(),
				};
	workload.generate_workload();
	debug!("\t\t************** Workload created with *******************");
	debug!("\t\tQuery distribution: {:?}", workload.query_distribution);
	debug!("\t\t#bulkwrites: {}", workload.n_bulkwrites);
	debug!("\t\t#gets: {}", workload.n_gets);
	debug!("\t\t#puts: {}", workload.n_puts);
	debug!("\t\tData distribution: {:?}", workload.data_distribution);
	debug!("\t\tLookup type: {:?}", workload.lookup_type);
	debug!("\t\t********************************************************");
}

impl Workload
{
	fn generate_get(&self, file: &mut File, data: &Vec<i32>, sampled_key: i32) {
		let mut _key: i32 = 0;
		if match self.lookup_type {LookupType::SINGLE_RESULT => 1, _ => 0} == 1 // SINGLE RESULT QUERIES
		{
			let index: usize = rand::thread_rng().gen_range(0, data.len()).try_into().unwrap();
			_key = data[index];
		}
		else 
		{
			_key = sampled_key;
		}
		if let Err(e) = writeln!(file, "g {}", _key) {
			error!("FAIL {}", e);
		}
	}

	fn generate_workload(&self)
	{
		let mut workload_file = setup_file(&self.workload_file);
		let mut bulkwrite_file = setup_file(&self.bulkwrite_file);

	    // ************************* Bulkwrite queries ************************* 
		let mut data: Vec<i32> = Vec::new();
		let mut rng = rand::thread_rng();
		if match self.data_distribution {Distribution::UNIFORM => 1, _ => 0} == 1 // UNIFORM DATA
		{
			let _uni_range = uniform::from(self.key_min_value..self.key_max_value);
			for _i in 0..self.n_bulkwrites 
			{
				let _key: i32 = _uni_range.sample(&mut rng);
				let _value: i32 = rng.gen_range(i32::min_value(), i32::max_value());
				data.push(_key);
				if let Err(e) = writeln!(bulkwrite_file, "b {} {}", _key, _value) {
			        error!("FAIL {}", e);
			    }
				//debug!("{}. [{}, {}]", _i+1, key, key); 
			}
		}
		else // SKEW DATA
		{
			let mean: i32 = ((self.key_min_value + i32::max_value())/2) as i32;
			let sd: i32 = 10000;
			let normal = normal::new(mean.into(), sd.into());
			for _i in 0..self.n_bulkwrites 
			{
				let _key: i32 = normal.sample(&mut rand::thread_rng()) as i32;
				let _value: i32 = rng.gen_range(i32::min_value(), i32::max_value());	
				data.push(_key);
				if let Err(e) = writeln!(bulkwrite_file, "b {} {}", _key, _value) {
			        error!("FAIL {}", e);
			    }
				//debug!("{}. [{}, {}]", _i+1, key, key); 
			}
		}

		if match self.query_distribution {Distribution::UNIFORM => 1, _ => 0} == 1 // UNIFORM QUERIES
		{
			let _uni_range = uniform::from(self.key_min_value..self.key_max_value);
			let mut _i: usize = 0;
			let mut _j: usize = 0;
			while _i < self.n_gets && _j < self.n_puts 
			{
				let gets_left = self.n_gets - _i;
				let puts_left = self.n_puts - _j;
				let is_get: bool = rng.gen_range(0, gets_left + puts_left) < gets_left;
				if is_get // generate a get query
				{
					self.generate_get(&mut workload_file, &data, _uni_range.sample(&mut rng));
					//debug!("{}. [{}]", _i, _key); 
					_i += 1;
				}
				else // generate a put query
				{
					let _key: i32 = _uni_range.sample(&mut rng);
					let _value: i32 = rng.gen_range(i32::min_value(), i32::max_value());
					data.push(_key);
					if let Err(e) = writeln!(workload_file, "p {} {}", _key, _value) {
				        error!("FAIL {}", e);
				    }
					//debug!("{}. [{}, {}]", _j, _key, key); 
					_j += 1;	
				}
			}
			while _i < self.n_gets
			{
				self.generate_get(&mut workload_file, &data, _uni_range.sample(&mut rng));
				//debug!("{}. [{}]", _i, _key); 
				_i += 1;
			}
			while _j < self.n_puts
			{
				let _key: i32 = _uni_range.sample(&mut rng);
				let _value: i32 = rng.gen_range(i32::min_value(), i32::max_value());
				data.push(_key);
				if let Err(e) = writeln!(workload_file, "p {} {}", _key, _value) {
				    error!("FAIL {}", e);
				}
				//debug!("{}. [{}, {}]", _j, _key, _key); 
				_j += 1;
			}
		}
		else // SKEW QUERIES
		{
			let mean: i32 = (self.key_min_value + self.key_max_value)/2;
			let sd: i32 = 10000;
			let normal = normal::new(mean.into(), sd.into());
			let mut _i: usize = 0;
			let mut _j: usize = 0;
			while _i < self.n_gets && _j < self.n_puts 
			{
				let gets_left = self.n_gets - _i;
				let puts_left = self.n_puts - _j;
				let is_get: bool = rng.gen_range(0, gets_left + puts_left) < gets_left;
				if is_get // generate a get query
				{
					self.generate_get(&mut workload_file, &data, normal.sample(&mut rand::thread_rng()) as i32);
					//debug!("{}. [{}]", _i, _key); 
					_i += 1;
				}
				else // generate a put query
				{
					let _key: i32 = normal.sample(&mut rand::thread_rng()) as i32;	
					let _value: i32 = rng.gen_range(i32::min_value(), i32::max_value());
					data.push(_key);
					if let Err(e) = writeln!(workload_file, "p {} {}", _key, _value) {
				        error!("FAIL {}", e);
				    }
					//debug!("{}. [{}, {}]", _j, _key, key); 
					_j += 1;	
				}
			}
			while _i < self.n_gets
			{
				self.generate_get(&mut workload_file, &data, normal.sample(&mut rand::thread_rng()) as i32);
				//debug!("{}. [{}]", _i, _key); 
				_i += 1;
			}
			while _j < self.n_puts
			{
				let _key: i32 = normal.sample(&mut rand::thread_rng()) as i32;	
				let _value: i32 = rng.gen_range(i32::min_value(), i32::max_value());
				data.push(_key);
				if let Err(e) = writeln!(workload_file, "p {} {}", _key, _value) {
				    error!("FAIL {}", e);
				}
				//debug!("{}. [{}, {}]", _j, _key, _key); 
				_j += 1;
			}
		}
	}
}

