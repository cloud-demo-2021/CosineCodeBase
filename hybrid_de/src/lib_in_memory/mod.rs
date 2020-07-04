//use std::mem;
use log::{debug};

use std::collections::{HashMap, HashSet};
use std::i32;
use crate::configuration::CONFIGURATION;
use super::lib_template::{Record};

use super::lib_helper::{bytes_to_records};

#[allow(non_upper_case_globals)]

pub struct MemoryBuffer {
	pub buffer_size: usize,
	pub buffer: HashMap<i32, String>,
	level: usize,
}

impl MemoryBuffer {
	pub fn create_buffer() -> MemoryBuffer {
		MemoryBuffer {
			buffer_size: CONFIGURATION.BUFFER_CAPACITY/CONFIGURATION.RECORD_SIZE, // #elements in the in-memory buffer
			buffer: HashMap::new(),
			level: 0,
		}
	}

	pub fn put(&mut self, key: &i32, value: &str) {
		self.buffer.insert(*key, value.to_string());
	}

	pub fn delete(&mut self, key: &i32) {
		self.buffer.insert(*key, "0".repeat(CONFIGURATION.RECORD_SIZE - CONFIGURATION.KEY_SIZE));
	}

	pub fn get(&self, key: &i32) -> Option<&String> {
		self.buffer.get(key)
	}

	pub fn find_range(&self, lower: &i32, upper: &i32) -> Vec<Record> {
		let mut range = Vec::new();
		for (key, value) in self.buffer.iter() {
			if key >= lower && key <= upper {
				range.push(Record::create_record(*key, value.clone()));
			}
		}
		range.sort();
		range
	}

	pub fn merge(&self) -> Vec<Record> {
		let mut records = Vec::new();
		let mut buffer_data: Vec<_> = self.buffer.clone().into_iter().collect();
		buffer_data.sort();
		for (key, value) in buffer_data.iter() {
			records.push(Record::create_record(*key, value.clone()));
		}
		records
	}

	pub fn flush(&mut self, data: Vec<u8>, _capacity: usize) {
		let records = bytes_to_records(&data);
		for record in records.iter() {
			self.buffer.insert(record.key, record.value.clone());
		}
	}

	pub fn size(&self) -> usize {
		self.buffer.len() * CONFIGURATION.RECORD_SIZE
	}

	pub fn capacity(&self) -> usize {
		CONFIGURATION.BUFFER_CAPACITY
	}

	pub fn level(&self) -> usize { 
		self.level 
	}

    pub fn is_full(&self) -> bool {
		assert!(self.buffer.len() <= self.buffer_size);
		//self.buffer.len() as f64 >= self.buffer_size as f64 * CONFIGURATION.PC_FULL_THRESHOLD
		self.buffer.len() >= self.buffer_size
	}

	pub fn clear(&mut self) { 
		self.buffer.clear();
	}

	pub fn print_stats(&self, distinct_keys: &mut HashSet<i32>) {
		for (key, value) in self.buffer.iter() {
			debug!("{}:{}:L{}", key, value, self.level);
			distinct_keys.insert(*key);
		}
		debug!("\n");
	}
}