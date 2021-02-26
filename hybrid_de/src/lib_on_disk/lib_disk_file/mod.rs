use std::convert::TryInto;
use std::fs::{File};
use std::io::{SeekFrom};
use std::io::prelude::*;
use std::time::{SystemTime, UNIX_EPOCH};

use crate::configuration::CONFIGURATION;
use crate::lib_helper::{bytes_to_records, binary_search_fp};
use crate::lib_template::{Record};
use crate::metrics::{GET_IO_COUNTER_FOR_READS, GET_IO_COUNTER_FOR_WRITES};
use std::sync::atomic::{AtomicBool, Ordering};

use bloom::{BloomFilter};

pub struct DiskFile {
    pub filename: String,
    //pub compacting: AtomicBool,
    pub size: usize,
    pub ts: SystemTime,
    pub fence_pointers: Vec<i32>,
    bloom_filter: BloomFilter,
}

impl DiskFile {

    pub fn create_disk_file(filename: String, data: &[u8], size: usize, ts: SystemTime) -> DiskFile {
        let mut fence_pointers: Vec<i32> = Vec::new();
        let mut bloom_filter = DiskFile::init_bloom_filter(size);
        //println!("creating file {}", &filename);

        // initialize bloom filters and fence pointers
        for i in (0..size).step_by(CONFIGURATION.RECORD_SIZE) {
            let key = i32::from_be_bytes(data[i..i + CONFIGURATION.KEY_SIZE].try_into().unwrap());
            //debug!("inserting {} to bloom filter", key);
            bloom_filter.insert(&key);
            // also keep track of max key in file
            if (i % CONFIGURATION.BLOCK_SIZE) == 0 || i == size - CONFIGURATION.RECORD_SIZE {
            //if (i % CONFIGURATION.BLOCK_SIZE) == 0 {
                fence_pointers.push(key);
            }
        }

        let mut file = File::create(&filename).expect(&format!("Failed to create file {}", &filename));
        let bytes_written = file.write(&data[..]).expect(&format!("Failed to write to file {}", &filename));

        assert!(size == bytes_written);

        DiskFile {
            filename: filename,
            size: size,
            ts: ts,
            //compacting: AtomicBool::new(false);
            fence_pointers: fence_pointers,
            bloom_filter: bloom_filter,
        }
    }

    pub fn get_score(&self) -> u64 {
        if CONFIGURATION.FILE_STRATEGY == "oldest_merged" || CONFIGURATION.FILE_STRATEGY == "oldest_flushed" {
            let since_the_epoch = self.ts.duration_since(UNIX_EPOCH).expect("Time went backwards");
            let in_ms = since_the_epoch.as_secs() * 1000 +
                since_the_epoch.subsec_nanos() as u64 / 1_000_000;
            return in_ms;
        }
        //println!("{}:{}", self.fence_pointers[0], self.fence_pointers.last().unwrap());
        let fence_diff = (*self.fence_pointers.last().unwrap() as i64 - self.fence_pointers[0] as i64) as u64;
        if CONFIGURATION.FILE_STRATEGY == "sparse_fp" {
            return fence_diff;
        } 
        if CONFIGURATION.FILE_STRATEGY == "dense_fp" {
            return u64::MAX - fence_diff;
        }
        0
    }

    pub fn get(&self, key: &i32) -> Option<String> {
        //debug!("filename is {}", &self.filename);
        //debug!("file contains {:?}", self.read_file(0, self.size));
        // if key bigger than max fence pointer
        if !self.bloom_filter.contains(key) || key > self.fence_pointers.last().unwrap() {
            //debug!("Not in bloom filter");
            return None;
        }
        let file_offset = match binary_search_fp(&self.fence_pointers, &key) {
            Some(idx) => {
                if idx == self.fence_pointers.len() - 1 {
                    idx - 1
                } else {
                    idx
                }
            },
            None => {
                //debug!("No offset found");
                return None;
            },
        };
        
        GET_IO_COUNTER_FOR_READS.inc();
        let records = self.read_file(file_offset * CONFIGURATION.BLOCK_SIZE, CONFIGURATION.BLOCK_SIZE);
        //debug!("records are {:?}", records);
        let idx = records.binary_search_by_key(key, |record| record.key);
        let idx = match idx {
            Err(_) => {
                //debug!("bloom filter false positive");
                return None;
            },
            Ok(v) => v,
        };
        //let idx = idx.unwrap();
        Some(records[idx].value.clone())
    }

    pub fn read_all_file_bytes(&self) -> Vec<u8> {
        let mut buffer = vec![0; self.size];
        let bytes_read = self.read_file_to_buffer(0, &mut buffer);
        assert!(bytes_read == self.size);
        buffer
    }

    pub fn read_all_file_records(&self) -> Vec<Record> {
        let records = bytes_to_records(&self.read_all_file_bytes());
        assert!(records[0].key == self.fence_pointers[0]);
        assert!(records.last().unwrap().key == *self.fence_pointers.last().unwrap());
        assert!(records.len() * CONFIGURATION.RECORD_SIZE == self.size);
        records
    }

    pub fn read_file(&self, start_offset: usize, num_bytes: usize) -> Vec<Record> {
        let mut buffer = vec![0; num_bytes];
        let bytes_read = self.read_file_to_buffer(start_offset, &mut buffer);
        let records = bytes_to_records(&buffer[..bytes_read]);
        let num_records = bytes_read / CONFIGURATION.RECORD_SIZE;
        //debug!("num records is {}, bytes read is {}, records len is {}", num_records, bytes_read, records.len());
        assert!(num_records == records.len());
        records
    }

    // pub fn write_file(&mut self, data: &[u8], size: usize) {
    //     let mut f = OpenOptions::new().append(true).open(&self.filename).expect("Failed to open file for appending");
    //     f.write_all(&data).expect("Failed to write file");
    //     // TODO: probably want to factor this out with create_disk_file
    //     if size > 0 {
    //         self.fence_pointers.pop();
    //     }
    //     for i in (0..size).step_by(CONFIGURATION.RECORD_SIZE) {
    //         let key = i32::from_be_bytes(data[i..i + CONFIGURATION.KEY_SIZE].try_into().unwrap());
    //         //debug!("inserting {} to bloom filter", key);
    //         info!("key is {}", key);
    //         self.bloom_filter.insert(&key);
    //         // also insert max fence pointer
    //         if (i % CONFIGURATION.BLOCK_SIZE) == 0 || i == size - CONFIGURATION.RECORD_SIZE {
    //             self.fence_pointers.push(key);
    //         }
    //     }
    //     self.size += size;
    // }

    pub fn read_file_to_buffer(&self, start_offset: usize, buffer: &mut[u8]) -> usize {
        let mut f = File::open(&self.filename).expect(&format!("Failed to open file {} for disk read", &self.filename));
        f.seek(SeekFrom::Start(start_offset as u64)).expect("Failed to seek file");
        f.read(&mut buffer[..]).expect("Failed to read file")
    }

    // private helper functions
    fn init_bloom_filter(size: usize) -> BloomFilter {
        let bf_records = size / CONFIGURATION.RECORD_SIZE;
        let bf_bits = CONFIGURATION.BF_BITS_PER_ENTRY * bf_records;
        let bf_hashes = bloom::optimal_num_hashes(bf_bits, bf_records as u32);
        BloomFilter::with_size(bf_bits, bf_hashes)
    }
}