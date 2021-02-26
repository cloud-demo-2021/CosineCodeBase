use std::cmp;
use std::collections::{HashSet};
use std::sync::{Arc};
use std::convert::TryInto;
use std::fs;
use std::fs::{File};
use std::io::prelude::*;
use std::time::{SystemTime};
use atomic_counter::{RelaxedCounter, AtomicCounter};
use std::sync::atomic::{AtomicBool, Ordering};

use log::{debug};

use crate::configuration::CONFIGURATION;
use crate::lib_helper::{generate_filename, bytes_to_records, binary_search_fp};
use crate::lib_template::{Record};
use super::lib_disk_file::{DiskFile};

//#[derive(Clone)]
pub struct Run {
    pub level: usize, // level run is on
    pub run: usize, // index of run in level (0, 1, 2, 3)
    pub size: usize, // bytes of data in run
    pub capacity: usize, // bytes of data run can hold
    pub file_counter: RelaxedCounter, 
    pub files: Vec<Arc<DiskFile>>,
    pub fence_pointers: Vec<i32>,
}

impl Clone for Run {
    fn clone(&self) -> Run {
        Run {
            level: self.level,
            run: self.run,
            size: self.size,
            capacity: self.capacity,
            file_counter: RelaxedCounter::new(self.file_counter.get()),
            files: self.files.clone(),
            fence_pointers: self.fence_pointers.clone(),
        }
    }
}

impl Run {
    // only used in buffer flush
	pub fn create_run(size: usize, capacity: usize, data: &[u8], level: usize, run: usize) -> Run {
        assert!(size > 0);
        let number_files = (size - 1) / CONFIGURATION.FILE_SIZE + 1;
        let mut fence_pointers: Vec<i32> = Vec::new();
        let mut files = Vec::new();
        let mut offset: usize = 0;

        //debug!("level {}, run {}", level, run);
        for i in 0..number_files {
            assert!(offset < size);
            let bytes_to_write = cmp::min(CONFIGURATION.FILE_SIZE, size - offset);
            let mut filename: &str = &format!("{}{}", CONFIGURATION.DB_PATH, generate_filename(level, run, i));
            debug!("creating file from create run {} in level {} run {} size is {}", filename, level, run, bytes_to_write);
            files.push(Arc::new(DiskFile::create_disk_file(filename.to_string(), &data[offset..offset + bytes_to_write], bytes_to_write, SystemTime::now())));
            
            let key = i32::from_be_bytes(data[offset..offset + CONFIGURATION.KEY_SIZE].try_into().unwrap());
            fence_pointers.push(key);

            offset += bytes_to_write;
        }
        assert!(offset == size);
        //debug!("my data is {:?}", bytes_to_records(data));

        Run {
            level: level,
            run: run,
            size: size,
            capacity: capacity,
            file_counter: RelaxedCounter::new(number_files),
            fence_pointers: fence_pointers,
            files: files,
		}
    }

    pub fn create_run_from_run(prev_run: &Run, capacity: usize, level: usize, run: usize) -> Run {
        debug!("create run from run: level is {}, run is {}", level, run);
        Run {
            level: level,
            run: run,
            size: prev_run.size,
            capacity: capacity,
            file_counter: RelaxedCounter::new(prev_run.file_counter.get()),
            fence_pointers: prev_run.fence_pointers.clone(),
            files: prev_run.files.clone(),
        }
    }

    pub fn create_empty_run(capacity: usize, level: usize, run: usize) -> Run {
        Run {
            level: level,
            run: run,
            size: 0,
            capacity: capacity,
            file_counter: RelaxedCounter::new(0), 
            fence_pointers: Vec::new(),
            files: Vec::new(),
        }
    }

    pub fn is_full(&self) -> bool {
        return self.size as f64 >= self.capacity as f64 * CONFIGURATION.FULL_THRESHOLD.min(CONFIGURATION.PC_FULL_THRESHOLD);
        //let num_runs = self.runs.len();
        //return num_runs > CONFIGURATION.RUNS_PER_LEVEL || (num_runs == CONFIGURATION.RUNS_PER_LEVEL && (self.runs[num_runs - 1].size as f64) / (self.runs[num_runs - 1].capacity as f64) > CONFIGURATION.FULL_THRESHOLD);
    }

    pub fn get_fullness(&self) -> f64 {
        return self.size as f64 / self.capacity as f64;
        //let num_runs = self.runs.len();
        //return num_runs > CONFIGURATION.RUNS_PER_LEVEL || (num_runs == CONFIGURATION.RUNS_PER_LEVEL && (self.runs[num_runs - 1].size as f64) / (self.runs[num_runs - 1].capacity as f64) > CONFIGURATION.FULL_THRESHOLD);
    }

    pub fn delete_files(&self) {
        for file in self.files.iter() {
            debug!("removing file {} in disk run, level is {}, run is {}", &file.filename, self.level, self.run);
            fs::remove_file(&file.filename).expect(&format!("Failed to remove file {} in disk run, level is {}, run is {}, file counter is {}, size is {}", &file.filename, self.level, self.run, self.file_counter.get(), self.size));
        }
    }

    pub fn remove_file_metadata(&mut self, files: Vec<Arc<DiskFile>>) {
        let fp_index = self.fence_pointers.binary_search(&files[0].fence_pointers[0]).unwrap();
        for file in files.iter() {
            // fp index and file index should be lined up
            assert!(self.fence_pointers[fp_index] == file.fence_pointers[0]);
            assert!(file.filename == self.files[fp_index].filename);
            self.size -= self.files[fp_index].size;
            self.files.remove(fp_index);
            self.fence_pointers.remove(fp_index);
        }
    }
    // this function should not return an empty vector
    pub fn get_files_in_fp(&self, min_fp: i32, max_fp: i32) -> &[Arc<DiskFile>] {
        // should be >= min_fp
        //debug!("min fp is {}, max fp is {}, fence pointers are {:?}", min_fp, max_fp, self.fence_pointers);
        let min_fp_idx = self.fence_pointers.binary_search(&min_fp).unwrap_or_else(|x| x);
        // idea is to find fp idx of the next largest key, and going up to that fp idx (not inclusive)
        let mut max_fp_idx = binary_search_fp(&self.fence_pointers, &(max_fp + 1)).unwrap();
        //debug!("max fp idx is {}, max fp in idx is {}", max_fp_idx, self.files[max_fp_idx].fence_pointers.last().unwrap());
        if *self.files[max_fp_idx].fence_pointers.last().unwrap() <= max_fp {
            max_fp_idx += 1;
        }
        //debug!("max fp idx is now {}", max_fp_idx);
        //debug!("min fp idx is now {}", min_fp_idx);
        // should NOT include max_fp_idx
        return &self.files[min_fp_idx..max_fp_idx];
    }

    // this function can return an empty vector
    pub fn get_files_containing_fp(&self, min_fp: i32, max_fp: i32) -> &[Arc<DiskFile>] {
        let min_fp_idx = match binary_search_fp(&self.fence_pointers, &min_fp) {
            Some(idx) => idx,
            None => 0,
        };
        let max_fp_idx = match binary_search_fp(&self.fence_pointers, &max_fp) {
            Some(idx) => idx,
            None => return &[], // empty if max is smaller than all entries
        };
        assert!(max_fp_idx >= min_fp_idx);
        return &self.files[min_fp_idx..max_fp_idx + 1];
    }

    // pub fn insert_file(&mut self, file_to_merge: Arc<DiskFile>) {
    //     // debug!("adding file {} to level {}, run {}", &file_to_merge.filename, self.level, self.run);
    //     // let fp_idx = self.fence_pointers.binary_search(&file_to_merge.fence_pointers[0]).unwrap_or_else(|x| x);
    //     // self.fence_pointers.insert(fp_idx, file_to_merge.fence_pointers[0]);
    //     // self.size += file_to_merge.size;
    //     // self.files.insert(fp_idx, file_to_merge);
    //     self.insert_files(vec![file_to_merge]);
    // }

    pub fn insert_files(&mut self, files_to_merge: Vec<Arc<DiskFile>>) {
        debug!("inserting {} files to level {}, run {}", files_to_merge.len(), self.level, self.run);
        let mut fp_idx = self.fence_pointers.binary_search(&files_to_merge[0].fence_pointers[0]).unwrap_or_else(|x| x);
        for i in 0..files_to_merge.len() {
            debug!("adding file {} to level {}, run {}", files_to_merge[i].filename, self.level, self.run);
            self.fence_pointers.insert(fp_idx, files_to_merge[i].fence_pointers[0]);
            self.size += files_to_merge[i].size;
            self.files.insert(fp_idx, files_to_merge[i].clone());
            fp_idx += 1;
        }
    }

    // pub fn insert_data(&mut self, data: Vec<u8>, min_fp: i32, ts: SystemTime) {
    //     let size = data.len();
    //     let mut fp_idx = self.fence_pointers.binary_search(&min_fp).unwrap_or_else(|x| x);
    //     let data_records = bytes_to_records(&data);
        
    //     if fp_idx + 1 < self.fence_pointers.len() {
    //         debug!("first is {}, second is {}", data_records.last().unwrap().key, self.fence_pointers[fp_idx + 1]);
    //         assert!(data_records.last().unwrap().key < self.fence_pointers[fp_idx + 1]);
    //     }
    //     let number_files = (size - 1) / CONFIGURATION.FILE_SIZE + 1;
    //     let mut offset: usize = 0;
    //     for i in 0..number_files {
    //         assert!(offset < size);
    //         let bytes_to_write = cmp::min(CONFIGURATION.FILE_SIZE, size - offset);
    //         let filename = generate_filename(self.level, self.run, self.file_counter + i);
    //         debug!("creating file {} from insert data in level {} run {}", filename, self.level, self.run);
    //         self.files.insert(fp_idx, Arc::new(DiskFile::create_disk_file(filename, &data[offset..offset + bytes_to_write], bytes_to_write, ts)));
            
    //         let key = i32::from_be_bytes(data[offset..offset + CONFIGURATION.KEY_SIZE].try_into().unwrap());
    //         self.fence_pointers.insert(fp_idx, key);
    //         assert!(self.files[fp_idx].fence_pointers[0] == self.fence_pointers[fp_idx]);

    //         offset += bytes_to_write;
    //         fp_idx += 1;
    //     }
    //     self.file_counter += number_files;
    //     self.size += size;
    //     assert!(offset == size);
    // }

    pub fn choose_files_to_merge(&self) -> Vec<Arc<DiskFile>> {
        debug!("number files is {}, level is {}, run is {}, size is {}", self.files.len(), self.level, self.run, self.size);
        let target_size = std::cmp::min(self.size, (CONFIGURATION.PC_LOWER_THRESHOLD * self.capacity as f64) as usize);
        //println!("my size is {}, target size is {}", self.size, target_size);
        let over_size = self.size - target_size;
        let num_files = std::cmp::max(1, (over_size + CONFIGURATION.FILE_SIZE - 1) / CONFIGURATION.FILE_SIZE);
        //println!("num files is {}", num_files);
        if CONFIGURATION.FILE_STRATEGY == "choose_first" {
            return self.files[0..num_files].to_vec();
        }
        let mut chosen = 0;
        let mut highest_score = 0;
        // loop {
        for i in 0..self.files.len() {
            // if self.files[i].compacting.load(Ordering::SeqCst) {
            //     continue;
            // }
            let file_score = self.files[i].get_score();
            if file_score > highest_score {
                chosen = i;
                highest_score = file_score;
            }
        }
        //     if !self.files[i].compacting.compare_and_swap(true, Ordering::SeqCst) {
        //         break;
        //     }
        // }
        let end_of_max = std::cmp::min(chosen + num_files, self.files.len());
        // for j in chosen + 1..end_of_max {
        //     if self.files[j].compacting.compare_and_swap(true, Ordering::SeqCst) {
        //         end_of_max = j;
        //         break;
        //     }
        // }
        let start_of_max = std::cmp::max(0, chosen - (chosen + num_files - end_of_max));
        // for k in range(start_of_max - 1, chosen).rev() {
        //     if self.files[j].compacting.compare_and_swap(true, Ordering::SeqCst) {
        //         start_of_max = k + 1;
        //         break;
        //     }
        // }
        self.files[start_of_max..end_of_max].to_vec()

        //self.files.iter().max_by(|x, y| x.get_score().cmp(&y.get_score())).unwrap().clone()
    }

    pub fn get(&self, key: &i32) -> Option<String> { 
        //debug!("level {}, run {}", self.level, self.run);
        let file_idx = match binary_search_fp(&self.fence_pointers, key) {
            Some(idx) => idx,
            None => {
                //debug!("No file found");
                return None;
            },
        };
        return self.files[file_idx].get(key);
    }

    pub fn get_all_bytes(&self) -> Vec<u8> {
        let mut offset = 0;
        let mut buffer = vec![0; self.size];
        //let mut records_in_run: Vec<Record> = Vec::new();
        for file in self.files.iter() {
            debug!("file is {}, size is {}", file.filename, file.size);
            offset += file.read_file_to_buffer(0, &mut buffer[offset..offset + file.size]);
        }
        debug!("level is {}, run is {}, offset is {}, size is {}", self.level, self.run, offset, self.size);
        assert!(offset == self.size);
        buffer
    }

    pub fn get_all_records(&self) -> Vec<Record> {
        bytes_to_records(&self.get_all_bytes())
    }

    pub fn find_range(&self, lower: &i32, upper: &i32) -> Vec<Record> {
        let mut range: Vec<Record> = Vec::new();
        let lower_file_idx = match binary_search_fp(&self.fence_pointers, &lower) {
            Some(idx) => idx,
            None => 0,
        };
        let lower_offset = match binary_search_fp(&self.files[lower_file_idx].fence_pointers, &lower) {
            Some(idx) => idx,
            None => 0,
        };
        let upper_file_idx = match binary_search_fp(&self.fence_pointers, &upper) {
            Some(idx) => idx,
            None => return range, // range is empty if upper limit is smaller than all entries
        };
        let upper_offset = match binary_search_fp(&self.files[upper_file_idx].fence_pointers, &upper) {
            Some(idx) => idx,
            None => return range, // range is empty if upper limit is smaller than all entries,
        };
        for i in lower_file_idx..upper_file_idx + 1 {
            let mut start_offset = 0;
            let mut end_offset = 0;
            if i == lower_file_idx {
                start_offset = lower_offset;
                end_offset = self.files[lower_file_idx].size;
            }
            if i == upper_file_idx {
                // want to read the page containing the upper offset
                end_offset = std::cmp::min(upper_offset + CONFIGURATION.BLOCK_SIZE, self.files[upper_file_idx].size);
            }
            let records = self.files[i].read_file(start_offset, end_offset - start_offset);
            let lower_idx = records.binary_search_by_key(lower, |record| record.key).unwrap_or_else(|x| x);
            let upper_idx = records.binary_search_by_key(upper, |record| record.key).unwrap_or_else(|x| x - 1) + 1;
            range.extend_from_slice(&records[lower_idx..upper_idx]);
        }
        range
    }

    pub fn print_stats(&self, distinct_keys: &mut HashSet<i32>) {    
        for file in self.files.iter() {
            let mut buffer = vec![0; file.size];

            let mut f = File::open(&file.filename).expect("Failed to open file in disk run get");
            f.read(&mut buffer[..]).expect("Failed to read file in print stats");
            let records = bytes_to_records(&buffer);
            for j in 0..records.len() {
                print!("{}:{}:L{} ", records[j].key, records[j].value, self.level);
                distinct_keys.insert(records[j].key);
            }
        }
    }
}


