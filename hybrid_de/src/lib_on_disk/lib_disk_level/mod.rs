use log::{debug};
use std::collections::HashSet;

use crate::configuration::CONFIGURATION;

use crate::lib_in_memory::{MemoryBuffer};
use crate::lib_helper::{records_to_bytes};
use crate::lib_template::{Record};
use crate::lib_merge::{merge_k_sorted, merge_from_files};
use super::lib_disk_run::{Run};
use super::lib_disk_file::{DiskFile};
use std::time::{SystemTime};
use std::sync::{Arc};
use parking_lot::{RwLock};
use atomic_counter::{RelaxedCounter, AtomicCounter};
use std::sync::atomic::{AtomicUsize, Ordering};
use crate::metrics::{PUT_IO_COUNTER};

pub struct DiskLevel {
    //pub lock: RwLock<i32>,
    pub level: usize, // immutable
    pub size: AtomicUsize, // bytes of data in level
    pub capacity: AtomicUsize,
    pub runs: RwLock<Vec<Run>>,
    pub run_counter: RelaxedCounter,
}

impl DiskLevel {
    pub fn empty_level(capacity_of_run: usize, level: usize) -> DiskLevel {
        debug!("calling empty level for level {}", level);
        DiskLevel {
            //lock: RwLock::new(0),
            level: level,
            size: AtomicUsize::new(0),
            capacity: AtomicUsize::new(capacity_of_run * CONFIGURATION.RUNS_PER_LEVEL),
            runs: RwLock::new(Vec::new()),
            run_counter: RelaxedCounter::new(0),
        }
    }

    pub fn create_level(files: Vec<Vec<Arc<DiskFile>>>, size_of_run: usize, capacity_of_run: usize, level: usize) -> DiskLevel {
        debug!("creating new level {}", level);
        let mut new_level = DiskLevel::empty_level(capacity_of_run, level);
        new_level.flush(files, size_of_run, size_of_run, capacity_of_run);
        new_level
    }

    // pub fn create_level(data: Vec<u8>, size_of_run: usize, capacity_of_run: usize, level: usize) -> DiskLevel {
    //     debug!("creating new level {}", level);
    //     let mut new_level = DiskLevel::empty_level(capacity_of_run, level);
    //     new_level.flush(data, size_of_run, size_of_run, capacity_of_run);
    //     new_level
    // }

    pub fn create_level_from_buffer(data: Vec<Record>, size: usize, capacity_of_run: usize, level: usize) -> DiskLevel {
        debug!("calling create level from buffer, level is {}", level);
        let mut new_level = DiskLevel::empty_level(capacity_of_run, level);
        new_level.flush_from_buffer(data, size, capacity_of_run);
        // run counter already incremented
        new_level
    }

    pub fn create_level_with_run(run: Run, capacity_of_run: usize, level: usize) -> DiskLevel {
        debug!("calling create level with run, level is {}", level);
        let mut new_level = DiskLevel::empty_level(capacity_of_run, level);
        let mut runs = new_level.runs.write();
        new_level.add_size(run.size);
        new_level.run_counter.inc();
        runs.push(run);
        drop(runs);
        new_level
    }

    pub fn is_full(&self) -> bool {
        let runs = self.runs.read();
        let num_runs = runs.len();
        // if self.level != 1 {
        //     //println!("level is {}", self.level);
        //     assert!(num_runs <= CONFIGURATION.RUNS_PER_LEVEL);
        // } 
        let last_run_full = num_runs == CONFIGURATION.RUNS_PER_LEVEL && runs[num_runs - 1].is_full();
        last_run_full || num_runs > CONFIGURATION.RUNS_PER_LEVEL || self.size() as f64 > self.capacity() as f64 * CONFIGURATION.PC_FULL_THRESHOLD
    }

    pub fn capacity(&self) -> usize {
        self.capacity.load(Ordering::Relaxed)
    }
    
    pub fn size(&self) -> usize {
        self.size.load(Ordering::Relaxed)
    }

    pub fn add_size(&self, sz: usize) {
        self.size.fetch_add(sz, Ordering::Relaxed);
    }

    pub fn dec_size(&self, sz: usize) {
        self.size.fetch_sub(sz, Ordering::Relaxed);
    }

    pub fn add_capacity(&self, sz: usize) {
        self.capacity.fetch_add(sz, Ordering::Relaxed);
    }

    pub fn dec_capacity(&self, sz: usize) {
        self.capacity.fetch_sub(sz, Ordering::Relaxed);
    }

	pub fn level(&self) -> usize {
        self.level
    }

    pub fn remove_run(&self, idx: usize) {
        let mut runs = self.runs.write();
        let run_size = runs[idx].size;
        self.dec_size(run_size);
        //println!("removing {} from level {}, remove run", run_size, self.level);
        runs.remove(idx);
    }

    pub fn add_run(&self, run: Run) {
        let mut runs = self.runs.write();
        self.add_size(run.size);
        runs.push(run);
    }

    pub fn replace_run(&self, run: Run, idx: usize) {
        let mut runs = self.runs.write();
        //println!("curr size of level {} is {}, run size is {}, num runs is {}", self.level, self.size(), runs[idx].size, runs.len());
        if runs.len() == 1 {
            assert!(runs[idx].size == self.size());
        }
        assert!(runs[idx].size <= self.size());
        self.dec_size(runs[idx].size);
        //println!("removing {} from level {}, replace run", runs[idx].size, self.level);
        self.add_size(run.size);
        //println!("adding {} to level {}", run.size, self.level);
        //println!("new size is {}", self.size());
        runs[idx] = run;
    }

    pub fn choose_run(&self) -> usize {
        let runs = self.runs.read();
        if CONFIGURATION.RUN_STRATEGY == "first" || self.level == 1 {
            return 0;
        }
        let mut fullest_run: usize = runs.len();
        let mut fullness = 0.;
        if CONFIGURATION.RUN_STRATEGY == "fullest" {
            for (i, run) in runs.iter().enumerate() {
                let run_fullness = run.get_fullness();
                if run_fullness > fullness {
                    fullest_run = i;
                    fullness = run_fullness;
                }
            }
            assert!(runs[fullest_run].get_fullness() == fullness);
            return fullest_run;
        }
        // run strategy should be "last_full"
        for (i, run) in runs.iter().enumerate().rev() {
            if run.is_full() {
                fullest_run = i;
                break;
            }
        }
        assert!(fullest_run != runs.len());
        assert!(runs[fullest_run].is_full());
        // debug!("fullest run is at idx {}, run id is {}, size is {}, capacity is {}, is full is {}", fullest_run, levels[level_num - 1].runs[fullest_run].run, levels[level_num - 1].runs[fullest_run].size, levels[level_num - 1].runs[fullest_run].capacity, levels[level_num - 1].runs[fullest_run].is_full());
        return fullest_run;
    }

    pub fn clear(&self) {
        let mut runs = self.runs.write();
        for run in runs.iter() {
            run.delete_files();
        }
        runs.clear();
        debug!("clear: I am level {}, I have {} runs", self.level, runs.len());
        self.size.store(0, Ordering::Relaxed);
    }

    pub fn get_all_files(&self) -> Vec<Vec<Arc<DiskFile>>> {
        debug!("get all files in level {}", self.level);
        let mut level_files = Vec::new();
        let runs = self.runs.read();
        for run in runs.iter() {
            assert!(run.files.len() > 0);
            level_files.push(run.files.clone());
        }
        level_files
    }
            

    pub fn flush_from_buffer(&self, data_records: Vec<Record>, size: usize, capacity_of_run: usize) {
        let data = records_to_bytes(&data_records);
        let mut num_flushed = 0;
        let mut runs = Vec::new();
        
        while num_flushed < size {
            debug!("creating run in level {}, run counter is {}", self.level, self.run_counter.get());
            //debug!("flushing run {} in level {}", self.runs.len(), self.level);
            let to_flush = std::cmp::min(std::cmp::max(capacity_of_run, CONFIGURATION.FILE_SIZE), size - num_flushed);
            let new_run = Run::create_run(to_flush, capacity_of_run, &data[num_flushed..num_flushed + to_flush], self.level, self.run_counter.get());
            PUT_IO_COUNTER.inc_by((to_flush as f64 / CONFIGURATION.BLOCK_SIZE as f64).ceil() as i64);
            num_flushed += to_flush;
            runs.push(new_run);
            self.run_counter.inc();
        }
        let mut original_runs = self.runs.write();
        self.add_size(size);
        //println!("adding {} to level 1 from flush buffer", size);
        original_runs.extend(runs);
    }


    // pub fn flush(&self, files: Vec<Vec<Arc<DiskFile>>>, data_size: usize, size_per_run: usize, capacity_of_run: usize) {
    //     let runs = self.runs.read();
    //     debug!("flushing data size {}", data_size);
    //     let num_runs = runs.len();
    //     if num_runs > 0 {
    //         let last_run = &runs[num_runs - 1];
    //         let last_run_size = last_run.size;
    //         assert!(last_run.capacity == capacity_of_run);
    //         // size per run used to ensure any run has size > FILESIZE
    //         if size_per_run > capacity_of_run || (last_run.size < capacity_of_run && !last_run.is_full()) {
    //             //debug!("level is {}, data size is {}, last run size is {}, capacity is {}", self.level, data_size, last_run.size, capacity_of_run);
    //             let mut all_files_merge = vec![last_run.files.clone()];
    //             //drop(guard); // drop reader lock

    //             all_files_merge.extend(files);
    //             let merged_files =  merge_from_files(all_files_merge, last_run, SystemTime::now());
    //             let mut files_size = 0;
    //             let mut file_idx = merged_files.len();
    //             let mut new_last_run = Run::create_empty_run(capacity_of_run, self.level, self.run_counter.get());
    //             debug!("calling create run for level {}, run {} here", self.level, self.run_counter.get());
    //             self.run_counter.inc();
    //             for i in 0..merged_files.len() {
    //                 files_size += merged_files[i].size;
    //                 if file_idx == merged_files.len() && files_size >= size_per_run {
    //                     file_idx = i;
    //                 }
    //             }
    //             debug!("file idx is {}, merged files length is {}, size per run is {}, files size is {}", file_idx, merged_files.len(), size_per_run, files_size);
    //             if file_idx == merged_files.len() {
    //                 file_idx = merged_files.len() - 1;
    //             }
    //             new_last_run.insert_files(merged_files[..file_idx + 1].to_vec());
                
    //             if merged_files.len() > file_idx + 1 {
    //                 //debug!("case 1");
    //                 debug!("calling create run for level {}, run {} here 2", self.level, self.run_counter.get());
    //                 let mut new_run = Run::create_empty_run(capacity_of_run, self.level, self.run_counter.get());
    //                 self.run_counter.inc();
    //                 new_run.insert_files(merged_files[file_idx + 1..].to_vec());

    //                 drop(runs);
    //                 let mut runs = self.runs.write();
    //                 self.add_size(files_size - last_run_size);
    //                 // last run
    //                 runs[num_runs - 1].delete_files();
    //                 runs[num_runs - 1] = new_last_run;
    //                 runs.push(new_run);
    //                 return;
    //             }
    //             //debug!("case 2");
    //             // a bit repetitive because of Rust ownership semantics
    //             drop(runs);
    //             let mut runs = self.runs.write();
    //             self.add_size(files_size - last_run_size);
    //             runs[num_runs - 1].delete_files();
    //             runs[num_runs - 1] = new_last_run;
    //             return;
    //         }
    //     }

    //     drop(runs);

    //     let mut counter = 1;
    //     let mut last_counter = 0;
    //     let mut files_size = 0;
    //     let mut empty_run = Run::create_empty_run(capacity_of_run, self.level, self.run_counter.get());
    //     self.run_counter.inc();
    //     let mut runs_to_add = Vec::new();
    //     let mut size_to_add = 0;
    //     let merged_files = merge_from_files(files, &mut empty_run, SystemTime::now());
    //     while counter < merged_files.len() + 1 {
    //         //debug!("flushing run {} in level {}", self.runs.len(), self.level);
    //         files_size += merged_files[counter - 1].size;
    //         if files_size >= size_per_run || counter == merged_files.len() {
    //             debug!("calling create run for level {}, run {}, size per run {}, capacity of run {}", self.level, self.run_counter.get(), size_per_run, capacity_of_run);
    //             let mut new_run = Run::create_empty_run(capacity_of_run, self.level, self.run_counter.get());
    //             new_run.insert_files(merged_files[last_counter..counter].to_vec());
    //             last_counter = counter;
    //             self.run_counter.inc();
    //             runs_to_add.push(new_run);
    //             size_to_add += files_size;
    //             files_size = 0;
    //         }
    //         counter += 1;
    //     }
    //     let mut runs = self.runs.write();
    //     runs.extend(runs_to_add);
    //     self.add_size(size_to_add);
    // }

    pub fn flush(&self, files: Vec<Vec<Arc<DiskFile>>>, data_size: usize, size_per_run: usize, capacity_of_run: usize) {
        let runs = self.runs.read();
        debug!("flushing data size {}", data_size);
        let num_runs = runs.len();
        if num_runs > 0 {
            let last_run = &runs[num_runs - 1];
            let last_run_size = last_run.size;
            assert!(last_run.capacity == capacity_of_run);
            // size per run used to ensure any run has size > FILESIZE
            if (CONFIGURATION.COMPACTION_STRATEGY == "partial" && self.level == CONFIGURATION.FULL_COMPACTION_LEVELS + 1 && num_runs == CONFIGURATION.RUNS_PER_LEVEL) || size_per_run > capacity_of_run || (last_run.size < capacity_of_run && !last_run.is_full()) {
                //debug!("level is {}, data size is {}, last run size is {}, capacity is {}", self.level, data_size, last_run.size, capacity_of_run);
                let mut all_files_merge = vec![last_run.files.clone()];
                //drop(guard); // drop reader lock

                all_files_merge.extend(files);
                let merged_files =  merge_from_files(all_files_merge, last_run, SystemTime::now());
                let mut files_size = 0;
                let mut file_idx = merged_files.len();
                let mut new_last_run = Run::create_empty_run(capacity_of_run, self.level, self.run_counter.get());
                debug!("calling create run for level {}, run {} here", self.level, self.run_counter.get());
                self.run_counter.inc();
                for i in 0..merged_files.len() {
                    files_size += merged_files[i].size;
                    if file_idx == merged_files.len() && files_size >= size_per_run {
                        file_idx = i;
                    }
                }
                debug!("file idx is {}, merged files length is {}, size per run is {}, files size is {}", file_idx, merged_files.len(), size_per_run, files_size);
                if file_idx == merged_files.len() || num_runs == CONFIGURATION.RUNS_PER_LEVEL {
                    file_idx = merged_files.len() - 1;
                }
                new_last_run.insert_files(merged_files[..file_idx + 1].to_vec());
                
                if merged_files.len() > file_idx + 1 {
                    //debug!("case 1");
                    debug!("calling create run for level {}, run {} here 2", self.level, self.run_counter.get());
                    let mut new_run = Run::create_empty_run(capacity_of_run, self.level, self.run_counter.get());
                    self.run_counter.inc();
                    new_run.insert_files(merged_files[file_idx + 1..].to_vec());

                    drop(runs);
                    let mut runs = self.runs.write();
                    self.add_size(files_size - last_run_size);
                    //println!("adding {} to level {}", files_size - last_run_size, self.level);
                    // last run
                    runs[num_runs - 1].delete_files();
                    runs[num_runs - 1] = new_last_run;
                    runs.push(new_run);
                    return;
                }
                //debug!("case 2");
                // a bit repetitive because of Rust ownership semantics
                drop(runs);
                let mut runs = self.runs.write();
                self.add_size(files_size - last_run_size);
                //println!("adding {} to level {}", files_size - last_run_size, self.level);
                runs[num_runs - 1].delete_files();
                runs[num_runs - 1] = new_last_run;
                return;
            }
        }

        drop(runs);

        let mut counter = 1;
        let mut last_counter = 0;
        let mut files_size = 0;
        let mut empty_run = Run::create_empty_run(capacity_of_run, self.level, self.run_counter.get());
        self.run_counter.inc();
        let mut runs_to_add = Vec::new();
        let mut size_to_add = 0;
        let merged_files = merge_from_files(files, &mut empty_run, SystemTime::now());
        while counter < merged_files.len() + 1 {
            //debug!("flushing run {} in level {}", self.runs.len(), self.level);
            files_size += merged_files[counter - 1].size;
            if (num_runs + runs_to_add.len() < CONFIGURATION.RUNS_PER_LEVEL && files_size >= size_per_run) || counter == merged_files.len() {
                debug!("calling create run for level {}, run {}, size per run {}, capacity of run {}", self.level, self.run_counter.get(), size_per_run, capacity_of_run);
                let mut new_run = Run::create_empty_run(capacity_of_run, self.level, self.run_counter.get());
                new_run.insert_files(merged_files[last_counter..counter].to_vec());
                last_counter = counter;
                self.run_counter.inc();
                runs_to_add.push(new_run);
                size_to_add += files_size;
                files_size = 0;
            }
            counter += 1;
        }
        let mut runs = self.runs.write();
        runs.extend(runs_to_add);
        self.add_size(size_to_add);
        //println!("adding {} to level {}", size_to_add, self.level);
    }

    pub fn get(&self, key: &i32) -> Option<String> {
        //debug!("get: I am level {}, I have {} runs", self.level, self.runs.len());
        let runs = self.runs.read();
        for run in runs.iter().rev() {
            match run.get(key) {
                Some(value) => {
                    return Some(value);
                },
                None => ()
            }
        }
        None
    }

    pub fn find_range(&self, lower: &i32, upper: &i32) -> Vec<Record> {
        let mut ranges = Vec::new();
        let runs = self.runs.read();
        for run in runs.iter() {
            ranges.push(run.find_range(lower, upper));
        }
        merge_k_sorted(ranges)
    }

    pub fn print_stats(&self, distinct_keys: &mut HashSet<i32>) {
        let runs = self.runs.read();
        for run in runs.iter() {
            run.print_stats(distinct_keys);
        }
        debug!("");
    }
}