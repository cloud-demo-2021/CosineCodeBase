use crate::configuration::CONFIGURATION;
use crate::lib_template::{Record};
use crate::lib_in_memory::{MemoryBuffer};
use crate::lib_helper::{records_to_bytes, generate_filename, bytes_to_records, binary_search_fp}; 
use super::lib_lsh_table::lib_disk_file::DiskFile;
use crate::metrics::PUT_IO_COUNTER;
use crate::metrics::{GET_IO_COUNTER_FOR_READS, GET_IO_COUNTER_FOR_WRITES};

use atomic_counter::{AtomicCounter};
use std::sync::atomic::{AtomicBool, Ordering};
use std::convert::TryInto;
use std::fs::{File};
use std::io::{SeekFrom};
use std::io::{Read};
use std::io;
use std::time::{SystemTime, UNIX_EPOCH};
use std::collections::{HashMap, HashSet};
use std::sync::{Arc};
use parking_lot::{RwLock, RwLockWriteGuard};
use log::{info, debug, error};

pub mod lib_disk_file;

pub struct LSHTable {
    pub buffer: RwLock<MemoryBuffer>,
    pub index: RwLock<HashMap<i32, usize>>,
    pub files: RwLock<Vec<DiskFile>>,
    pub flushing: AtomicBool, // true if thread is currently flushing buffer, false otherwise
}

impl LSHTable {
    pub fn create_lshtable() -> LSHTable {
        let buffer = RwLock::new(MemoryBuffer::create_buffer());
        LSHTable {
            buffer: buffer,
            index: RwLock::new(HashMap::new()),
            files: RwLock::new(Vec::new()),
            flushing: AtomicBool::new(false),
        }
    }

    pub fn get(&self, key: &i32) -> Option<String> {
        let target_file_object: &DiskFile;
        let value_on_disk: String;
        let buffer = self.buffer.read();
        match buffer.get(key) {
            Some(value) => {
                if value == &"0".repeat(CONFIGURATION.RECORD_SIZE - CONFIGURATION.KEY_SIZE) {
                    return None;
                }
                // println!("Key: {}, value: {}", key.to_string(), value.to_string()); 
                return Some(value.to_string());
            },
            None => {
                // check hash index - if found, key exists on disk
                let index = self.index.read();
                match index.get(key) {
                    Some(value) => {

                        // Determine the correct file object
                        let files = self.files.read();
                        target_file_object = &files[*value];

                        // Determine the correct block of file
                        let file_offset = match binary_search_fp(&target_file_object.fence_pointers, &key) {
                            Some(idx) => {
                                if idx == &target_file_object.fence_pointers.len() - 1 {
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
                        let records = &target_file_object.read_file(file_offset * CONFIGURATION.BLOCK_SIZE, CONFIGURATION.BLOCK_SIZE);
                        let idx = records.binary_search_by_key(key, |record| record.key);
                        let idx = match idx {
                            Err(_) => {
                                //debug!("hash table false positive");
                                return None;
                            },
                            Ok(v) => v,
                        };
                        value_on_disk = records[idx].value.clone();
                        // println!("Key: {}, value: {}", key.to_string(), value_on_disk); 
                        return Some(value_on_disk)
                    },
                    None => { 
                        println!("Not Found key {}", key.to_string()); 
                        return None
                    }
                };
            }
        }
    }

    pub fn put(&self, key: &i32, value: &str) {
        let target_file: String;
        let target_file_object: &DiskFile;
        let file_offset: usize;
        let index = self.index.read();
        match index.get(key) {
            Some(value) => {
                //info!("Update");

                // Get the correct file object
                let files = self.files.read();
                target_file_object = &files[*value];

                // Determine the correct block within file
                match binary_search_fp(&target_file_object.fence_pointers, &key) {
                    Some(idx) => {
                        if idx == &target_file_object.fence_pointers.len() - 1 {
                            file_offset = idx - 1;
                        } else {
                            file_offset = idx;
                        }

                        // Read the records for that offset
                        GET_IO_COUNTER_FOR_WRITES.inc();
                        let records = &mut target_file_object.read_file(file_offset * CONFIGURATION.BLOCK_SIZE, CONFIGURATION.BLOCK_SIZE);
                        let idx = records.binary_search_by_key(key, |record| record.key);
                        match idx {
                            Ok(v) => {
                                let index_position = v;
                                records[index_position].value = value.to_string();
                                &mut target_file_object.write_to_file(file_offset * CONFIGURATION.BLOCK_SIZE, &records_to_bytes(records));
                                PUT_IO_COUNTER.inc();
                            }
                            Err(_) => {
                                debug!("hash table false positive");
                            },
                        };
                    },
                    None => {debug!("No offset found");},
                };
            }
            None => { 
                //info!("Insert or update in place within buffer");
                drop(index);
                let mut buffer = self.buffer.write();
                buffer.put(key, value);
          
                if buffer.is_full() {
                    if !self.flushing.compare_and_swap(false, true, Ordering::Relaxed) {
                        self.flush_buffer_with_guard(buffer);
                    }
                    debug!("Flush complete!");
                }
            }
        }
    }


    pub fn read_modify_write(&self, key: &i32) {
        let target_file: String;
        let target_file_object: &DiskFile;
        let file_offset: usize;
        let index = self.index.read();
        match index.get(key) {
            Some(value) => {

                // Get the correct file object
                let files = self.files.read();
                target_file_object = &files[*value];

                // Determine the correct block within file
                match binary_search_fp(&target_file_object.fence_pointers, &key) {
                    Some(idx) => {
                        if idx == &target_file_object.fence_pointers.len() - 1 {
                            file_offset = idx - 1;
                        } else {
                            file_offset = idx;
                        }

                        // Read the records for that offset
                        GET_IO_COUNTER_FOR_WRITES.inc();
                        let records = &mut target_file_object.read_file(file_offset * CONFIGURATION.BLOCK_SIZE, CONFIGURATION.BLOCK_SIZE);
                        let idx = records.binary_search_by_key(key, |record| record.key);
                        match idx {
                            Ok(v) => {
                                let index_position = v;
                                records[index_position].value = ((&records[index_position].value.to_string()).trim().parse::<i32>().unwrap() + 1).to_string();
                                &mut target_file_object.write_to_file(file_offset * CONFIGURATION.BLOCK_SIZE, &records_to_bytes(records));
                                PUT_IO_COUNTER.inc();
                            }
                            Err(_) => {
                                debug!("hash table false positive");
                            },
                        };
                    },
                    None => {error!("ERROR, KEY FOR RMW OPCODE NOT FOUND");},
                };
            }
            None => { 
                error!("ERROR, KEY FOR RMW OPCODE NOT FOUND");
            }
        }
    }

    pub fn flush_buffer_with_guard(&self, mut buffer: RwLockWriteGuard<MemoryBuffer>) {
        // all of this must happen atomically to make sure no new data gets cleared
        let records = buffer.merge();
        let size = records.len();

        let data = records_to_bytes(&records);
        let mut num_flushed = 0;
        let mut files = self.files.write();

        while num_flushed < size {
            let max_file_id = files.len();
            let mut filename: &str = &format!("{}{}", CONFIGURATION.DB_PATH, generate_filename(0, 0, max_file_id as usize));
            let to_flush = std::cmp::min(CONFIGURATION.FILE_SIZE/CONFIGURATION.RECORD_SIZE, size - num_flushed);
            let new_file = DiskFile::create_disk_file(filename.to_string(), &data, data.len(), SystemTime::now());
            PUT_IO_COUNTER.inc_by((to_flush as f64 / CONFIGURATION.BLOCK_SIZE as f64).ceil() as i64);
            debug!("{} bytes flushed => {} I/Os", to_flush, (to_flush as f64 / CONFIGURATION.BLOCK_SIZE as f64).ceil() as i64);
            num_flushed += to_flush;
            
            files.push(new_file);

            //add buffer entries to the hash index
            let mut index = self.index.write();
            for (_key, _val) in &buffer.buffer
            {
                index.insert(*_key, files.len()-1);
            }
        }
        buffer.clear();
        self.flushing.store(false, Ordering::Relaxed);
    }

    // pub fn flush_from_buffer(&mut self, data_records: Vec<Record>, size: usize) {
    //     let data = records_to_bytes(&data_records);
    //     let mut num_flushed = 0;

    //     while num_flushed < size {
    //         let max_file_id = self.files.len();
    //         let filename: &str = &format!("{}{}", db_path, generate_filename(0, 0, max_file_id as usize));
    //         let to_flush = std::cmp::min(CONFIGURATION.FILE_SIZE/CONFIGURATION.RECORD_SIZE, size - num_flushed);
    //         let new_file = DiskFile::create_disk_file(filename.to_string(), &data, data.len(), SystemTime::now());
    //         PUT_IO_COUNTER.inc_by((to_flush as f64 / CONFIGURATION.BLOCK_SIZE as f64).ceil() as i64);
    //         debug!("{} bytes flushed => {} I/Os", to_flush, (to_flush as f64 / CONFIGURATION.BLOCK_SIZE as f64).ceil() as i64);
    //         num_flushed += to_flush;
    //         self.files.push(new_file);

    //         // add buffer entries to the hash index
    //         // for (_key, _val) in &mut self.buffer.buffer
    //         // {
    //         //     self.index.insert(*_key, self.files.len()-1);
    //         // }
    //     }
    // }

    pub fn print_stats(&self) {    
        let buffer = self.buffer.read();
        let index = self.index.read();
        println!("Buffer size: {:?} records", buffer.buffer_size);
        let files = self.files.read();
        if files.len() == 0{
            println!("No on-disk files created yet!");
        }
        else
        {
            println!("On disk files: {:?}", files.len());
            println!("Hash table contains {:?} entries.", index.keys().len());
        }
    }

}

