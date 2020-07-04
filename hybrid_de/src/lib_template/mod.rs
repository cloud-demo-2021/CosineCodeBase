use std::cmp::Ordering;
use crate::lib_helper::pad_string;
use crate::configuration::CONFIGURATION;

#[allow(non_snake_case)]
#[derive(Debug, Clone)]
#[derive(Eq, Hash)]
pub struct Record
{
	pub key: i32,
	pub value: String,
}

impl Record {
	pub fn create_record(key: i32, value: String) -> Record 
	{
		Record {
			key: key,
			value: pad_string(&value, CONFIGURATION.RECORD_SIZE - CONFIGURATION.KEY_SIZE),
		}
	}
}

// Don't want to compare values at all, only keys!
impl PartialOrd for Record {
    fn partial_cmp(&self, other: &Record) -> Option<Ordering> {
        Some(self.cmp(other))
    }
}

impl Ord for Record {
    fn cmp(&self, other: &Record) -> Ordering {
        self.key.cmp(&other.key)
    }
}

impl PartialEq for Record {
    fn eq(&self, other: &Record) -> bool {
        self.key == other.key
    }
}

// pub trait Level {
// 	fn get(&self, key: &i32, record: &mut Record) -> bool;
// 	fn find_range(&self, lower: &i32, upper: &i32) -> Vec<Record>;

// 	fn merge(&mut self) -> Vec<u8>;
// 	fn flush(&mut self, data: Vec<u8>, size_of_run: usize, capacity: usize);

// 	fn is_full(&self) -> bool;
// 	fn size(&self) -> usize;
// 	fn capacity(&self) -> usize;
// 	fn level(&self) -> usize;
// 	fn clear(&mut self);
// 	fn print_stats(&self, distinct_keys: &mut HashSet<i32>);
// }

// #[allow(non_snake_case)]
// pub struct Config
// {
// 	pub BLOCK_SIZE: usize,
// 	pub FILE_SIZE: usize,
// 	pub KEY_SIZE: usize,
// 	pub RECORD_SIZE: usize,
// 	pub BUFFER_CAPACITY: usize,
// 	pub SIZE_RATIO: u8, // T
// 	pub RUNS_PER_LEVEL: u8, // K
	
// 	// we eventually want to change this(?) with Monkey
// 	pub BF_BITS_PER_ENTRY: usize,
// }

// pub fn initialize_configuration() -> Config
// {
// 	let configuration = Config
// 	{
// 		BLOCK_SIZE: 4096, // 4 KB 
// 		FILE_SIZE: 12288, // 12 KB 
// 		KEY_SIZE: 4,
// 		RECORD_SIZE: 8, // 4+4 byte
// 		BUFFER_CAPACITY: 24576, // 24 KB
// 		SIZE_RATIO: 4, 
// 		RUNS_PER_LEVEL: 4, 

// 		BF_BITS_PER_ENTRY: 2,
// 	};
// 	debug!("\n\t\tInitialized configuration!");
// 	configuration
// }
