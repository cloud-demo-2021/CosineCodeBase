use std::collections::{HashSet};
use log::{debug};
use schedule_recv::{periodic_ms};

use std::sync::{Arc};
use parking_lot::{RwLock, RwLockWriteGuard};
use std::i32;
use std::fs;

use crate::configuration::CONFIGURATION;

use crate::lib_template::{Record};
use crate::lib_merge::{merge_k_sorted, merge_from_files};
use crate::lib_on_disk::lib_disk_run::{Run};
use crate::lib_on_disk::lib_disk_file::{DiskFile};
use crate::lib_on_disk::lib_disk_level::{DiskLevel};
use crate::lib_in_memory::{MemoryBuffer};
//use crate::metrics::{PC_LEVELS_COUNTER};
use std::time::{SystemTime};

use atomic_counter::{AtomicCounter};
use std::sync::atomic::{AtomicBool, Ordering};


pub struct LSMTree {
    pub buffer: RwLock<MemoryBuffer>,
    pub levels: RwLock<Vec<DiskLevel>>,
    pub compacting: AtomicBool, // true if thread is currently compacting, false otherwise
    //pub compacting_levels: Vec<AtomicBool>,
    pub flushing: AtomicBool, // true if thread is currently flushing buffer, false otherwise
}

impl LSMTree {
    pub fn create_lsmtree() -> LSMTree {
        let buffer = RwLock::new(MemoryBuffer::create_buffer());
        LSMTree {
            buffer: buffer,
            levels: RwLock::new(Vec::new()),
            compacting: AtomicBool::new(false),
            //compacting_levels: Vec::new(),
            flushing: AtomicBool::new(false),
        }
    }
    
    pub fn put(&self, key: &i32, value: &str) {
        let mut buffer = self.buffer.write();
        buffer.put(key, value);
        if buffer.is_full() {
            if !self.flushing.compare_and_swap(false, true, Ordering::Relaxed) {
                self.flush_buffer_with_guard(buffer);
            }
            self.compaction();
        }
    }

    pub fn flush_buffer_with_guard(&self, mut buffer: RwLockWriteGuard<MemoryBuffer>) {
        // all of this must happen atomically to make sure no new data gets cleared
        //let mut buffer = self.buffer.write();
        let data = buffer.merge();
        let mut new_capacity = CONFIGURATION.BUFFER_CAPACITY * CONFIGURATION.SIZE_RATIO / CONFIGURATION.RUNS_PER_LEVEL;
        let buffer_size = buffer.size();

        let levels = self.levels.read();
        if levels.len() > 0 {
            levels[0].flush_from_buffer(data, buffer_size, new_capacity as usize);
            // let _guard = levels[0].lock.write();
            // levels[0].extend(new_runs);
            // levels[0].size += runs_size;
            drop(levels);
        } else {
            drop(levels);
            let new_level = DiskLevel::create_level_from_buffer(data, buffer_size, new_capacity as usize, 1);
            // probably want to lock here
            //drop(guard);
            let mut levels = self.levels.write();
            levels.push(new_level);
            drop(levels);
        }
        buffer.clear();
        self.flushing.store(false, Ordering::Relaxed);
    }

    pub fn compaction(&self) {
        // if was true before, don't compact
        if self.compacting.compare_and_swap(false, true, Ordering::Relaxed) {
            return;
        }
        assert!(self.compacting.load(Ordering::Relaxed));
        if CONFIGURATION.COMPACTION_STRATEGY == "partial" {
            self.full_compaction_helper(CONFIGURATION.FULL_COMPACTION_LEVELS);
            if CONFIGURATION.COMPACTION_THREADS == 0 {
                self.partial_compaction_single();
            }
        } else {
            // should be "full"
            self.full_compaction();
        }
        assert!(self.compacting.load(Ordering::Relaxed));
        self.compacting.store(false, Ordering::Relaxed);
    }

    pub fn partial_compaction_single(&self) {
        loop {
            let level = self.choose_level_to_compact();
            if level == 0 {
                break;
            }
            self.choose_runs_to_compact(level);
        }
    }

    // pub fn partial_compaction(&self) {
    //     //println!("partial compaction");
    //     let pool = ThreadPool::new(CONFIGURATION.COMPACTION_THREADS);
    //     let tick = schedule_recv::periodic_ms(CONFIGURATION.PC_FREQUENCY);
    //     loop {
    //         tick.recv().unwrap();
    //         for i in 0..CONFIGURATION.COMPACTION_THREADS {
    //             pool.execute(move || {
    //                 let level = self.choose_level_to_compact();
    //                 self.choose_runs_to_compact(level);
    //                 self.compacting_levels[level - 1].store(false, Ordering::SeqCst);
    //             })
    //         }
    //         pool.join();
    //     }
    // }

    pub fn choose_level_to_compact(&self) -> usize {
        let mut fullest_level: usize = 0;
        let mut fullness: f64 = 0.; 
        //let _guard = levels_lock.read();
        let levels = self.levels.read();
        for level in levels[CONFIGURATION.FULL_COMPACTION_LEVELS..].iter() {
            // if level.is_full() {
            //     return level.level;
            // }
            debug!("level is {}, size is {}", level.level, level.size());
            let level_fullness: f64 = level.size() as f64 / level.capacity() as f64;
            if level.is_full() && level_fullness > fullness {
            //if level.is_full() && level_fullness > fullness && !self.compacting_levels[level.level - 1].load(Ordering::SeqCst) {
                fullest_level = level.level;
                fullness = level_fullness;
            }
        }
        // if fullest_level != 0 {
        //     self.compacting_levels[fullest_level - 1].store(true, Ordering::SeqCst);
            
        // }
        return fullest_level;
    }

    pub fn choose_runs_to_compact(&self, level_num: usize) {
        //PC_LEVELS_COUNTER.inc();
        let levels = self.levels.read();
        //println!("compacting level {}, level size is {}, level capacity is {}", level_num, levels[level_num - 1].size(), levels[level_num - 1].capacity());
        
        let fullest_run = levels[level_num - 1].choose_run();
        if fullest_run != 0 {
            //debug!("case 1");

            let level_runs = levels[level_num - 1].runs.read();
            let mut first_run_copy = level_runs[fullest_run - 1].clone();
            let mut second_run_copy = level_runs[fullest_run].clone();
            //drop(guard);

            drop(level_runs);
            drop(levels);

            let files_to_remove = self.compact_runs(&mut first_run_copy, &mut second_run_copy);
            // want to lock here

            let levels = self.levels.read();
            levels[level_num - 1].replace_run(first_run_copy, fullest_run - 1);
            if second_run_copy.size == 0 {
                levels[level_num - 1].remove_run(fullest_run);
            } else {
                levels[level_num - 1].replace_run(second_run_copy, fullest_run);
            }
            // end locking

            for file in files_to_remove.iter() {
                //println!("removing file {} in partial compaction, level is {}", &file.filename, level_num);
                fs::remove_file(&file.filename).expect(&format!("Failed to remove file {}", &file.filename));
            }
            return;
        }

        // fullest_run is 0
        // don't need to create new level
        if levels.len() > level_num {
            let first_level_runs = levels[level_num].runs.read();
            let num_runs = first_level_runs.len();
            if num_runs > 0 {
                // runs in next level
                let last_run = &first_level_runs[num_runs - 1];
                if num_runs == CONFIGURATION.RUNS_PER_LEVEL || (level_num == levels.len() - 1 && num_runs == CONFIGURATION.RUNS_LAST_LEVEL) || !last_run.is_full() {
                    //debug!("case 2");
                    let mut first_run_copy = last_run.clone();
                    //drop(guard);

                    let level_runs = levels[level_num - 1].runs.read();
                    let mut second_run_copy = level_runs[fullest_run].clone();
                    //drop(guard);

                    drop(first_level_runs);
                    drop(level_runs);
                    drop(levels);
                    let files_to_remove = self.compact_runs(&mut first_run_copy, &mut second_run_copy);

                    let levels = self.levels.read();
                    levels[level_num].replace_run(first_run_copy, num_runs - 1);
                    if second_run_copy.size == 0 {
                        levels[level_num - 1].remove_run(fullest_run);
                    } else {
                        levels[level_num - 1].replace_run(second_run_copy, fullest_run);
                    }

                    // end locking
                    for file in files_to_remove.iter() {
                        //println!("removing file {} in partial compaction, level is either {} or {}", &file.filename, level_num, level_num + 1);
                        fs::remove_file(&file.filename).expect(&format!("Failed to remove file {}", &file.filename));
                    }
                    return;
                }
            }
        }

        drop(levels);
        let levels = self.levels.read();
        // create new run -- fullest_run is 0 here
        let mut new_capacity = CONFIGURATION.BUFFER_CAPACITY * CONFIGURATION.SIZE_RATIO.pow((level_num + 1) as u32) / CONFIGURATION.RUNS_PER_LEVEL;

        let level_runs = levels[level_num - 1].runs.read();
        let mut new_run = Run::create_run_from_run(&level_runs[fullest_run], new_capacity as usize, level_num + 1, 0);

        if levels.len() == level_num {
            drop(level_runs);
            drop(levels);
            //debug!("case 3, run size is {}, current capacity is {}, new capacity is {}", run_size, levels[level_num - 1].runs[fullest_run].capacity, new_capacity);
            let new_level = DiskLevel::create_level_with_run(new_run, new_capacity as usize, level_num + 1);
            // want to lock here probably
    
            let mut levels = self.levels.write();
            levels.push(new_level);
            levels[level_num - 1].remove_run(fullest_run);
        } else {
            //debug!("case 4");
            new_run.run = levels[level_num].run_counter.get();
            levels[level_num].run_counter.inc();
            // want to lock here probably
            debug!("reassigning created run to run number {}", new_run.run);
            drop(level_runs);

            levels[level_num].add_run(new_run);
            levels[level_num - 1].remove_run(fullest_run);

            drop(levels);
        }
    }

    pub fn partial_merge_helper(&self, overlapping_files: Vec<Arc<DiskFile>>, newer_files: Vec<Arc<DiskFile>>, run_merge_into: &mut Run) -> (usize, Vec<Arc<DiskFile>>) {
        let mut files_to_remove = Vec::new();
        files_to_remove.extend(overlapping_files.clone());
        files_to_remove.extend(newer_files.clone());
        run_merge_into.remove_file_metadata(overlapping_files.clone());
        let ts = if CONFIGURATION.FILE_STRATEGY == "oldest_flushed" {
            // newest file timestamp
            files_to_remove.iter().max_by(|x, y| x.ts.cmp(&y.ts)).unwrap().clone().ts
        } else {
            SystemTime::now()
        };
        let merged_files = merge_from_files(vec![overlapping_files, newer_files], run_merge_into, ts);
        let mut data_size = 0;
        for file in merged_files.iter() {
            data_size += file.size;
        }
        run_merge_into.insert_files(merged_files);
        return (data_size, files_to_remove);
    }
    
    pub fn compact_runs(&self, older: &mut Run, newer: &mut Run) -> Vec<Arc<DiskFile>> {
        let files_to_merge = newer.choose_files_to_merge();
        let min_fp = files_to_merge[0].fence_pointers[0];
        let max_fp = *files_to_merge.last().unwrap().fence_pointers.last().unwrap();
        let overlapping_files = older.get_files_containing_fp(min_fp, max_fp);
        // for file in overlapping_files {
        //     if file.compacting.load(Ordering::SeqCst) {
        //         return Vec::new();
        //     }
        // }
        if overlapping_files.len() == 0 {
            let files_size = files_to_merge.iter().fold(0, |mut sum, file| {sum += file.size; sum});
            let levels = self.levels.read();
            //TODO: move this elsewhere
            older.insert_files(files_to_merge.clone());
            newer.remove_file_metadata(files_to_merge);
            // levels[newer.level - 1].dec_size(files_size);
            //println!("removing {} from level {}, 1", files_size, newer.level);
            drop(levels);
            return Vec::new();
        }
        let mut overlapping_size = overlapping_files.iter().fold(0, |mut sum, file| {sum += file.size; sum});
        let min_overlapping_fp = overlapping_files[0].fence_pointers[0];
        let max_overlapping_fp = *overlapping_files.last().unwrap().fence_pointers.last().unwrap();

        let newer_files = newer.get_files_in_fp(std::cmp::min(min_overlapping_fp, min_fp), std::cmp::max(max_overlapping_fp, max_fp)).to_vec();
        //println!("actual newer files is {}", newer_files.len());
        let mut found = false;
        let mut newer_size = 0;

        let newer_size = newer_files.iter().fold(0, |mut sum, file| {sum += file.size; sum});
        
        debug!("newer run is {}, remove metadata", newer.run);

        // a bit redundant bc of Rust ownership semantics
        let (data_size, files_to_remove) = 
            self.partial_merge_helper(overlapping_files.to_vec(), newer_files.to_vec(), older);

        newer.remove_file_metadata(newer_files);

        //debug!("after: newer size is {}", newer.size);
        return files_to_remove;
    }

    fn prev_size_new_capacity(&self, level_idx: usize) -> (usize, usize) {
        let prev_size = if level_idx == 0 {
            let buffer = self.buffer.read();
            buffer.size()
        } else {
            let levels = self.levels.read();
            levels[level_idx - 1].size()
        }; 
        let new_capacity = CONFIGURATION.BUFFER_CAPACITY * CONFIGURATION.SIZE_RATIO.pow((level_idx + 1) as u32) / CONFIGURATION.RUNS_PER_LEVEL;
        (prev_size, new_capacity)
    }

    fn clear_prev_level(&self, level_idx: usize) {
        if level_idx == 0 {
            let mut buffer = self.buffer.write();
            buffer.clear();
        } else {
            let levels = self.levels.read();
            debug!("clearing level at idx {}, level is {}", level_idx - 1, levels[level_idx - 1].level);
            levels[level_idx - 1].clear();
        }
    }

    pub fn full_compaction(&self) {
        let mut levels = self.levels.read();
        let num_levels = levels.len();
        drop(levels);
        self.full_compaction_helper(num_levels);
    }

    pub fn flush_files(&self, i: usize, level_files: Vec<Vec<Arc<DiskFile>>>) {
        let (prev_size, new_capacity) = self.prev_size_new_capacity(i);
        let levels = self.levels.read();

        if i == levels.len() - 1 {
            levels[i].flush(level_files.clone(), prev_size, std::cmp::max(CONFIGURATION.FILE_SIZE, (new_capacity as f64 * CONFIGURATION.RUNS_PER_LEVEL as f64 / CONFIGURATION.RUNS_LAST_LEVEL as f64) as usize), new_capacity);
        } else {
            levels[i].flush(level_files.clone(), prev_size, std::cmp::max(CONFIGURATION.FILE_SIZE, new_capacity), new_capacity);
        }

        drop(levels);
        self.clear_prev_level(i);
    }

    pub fn full_compaction_helper(&self, num_levels: usize) {
        //self.flush_buffer();
        let mut levels = self.levels.read();
        let mut level_files = Vec::new();
        for i in 0..num_levels {
            debug!("merge and flush: I am level {} at idx {}", levels[i].level, i);
            if level_files.len() > 0 {
                drop(levels);
                self.flush_files(i, level_files);
                levels = self.levels.read();
            }

            if !levels[i].is_full() {
                debug!("finished merge and flush");
                return;
            }
            
            debug!("attempting to merge data at idx {}, level is {}", i, levels[i].level);
            level_files = levels[i].get_all_files();
        }

        // need to create new level
        if level_files.len() > 0 {
            let level_len = levels.len();
            if num_levels == level_len {
                debug!("creating new level");

                drop(levels);
                let (_prev_size, new_capacity) = self.prev_size_new_capacity(level_len);
                let mut levels = self.levels.write();

                levels.push(DiskLevel::create_level(level_files, std::cmp::max(CONFIGURATION.FILE_SIZE, (new_capacity as f64 * CONFIGURATION.RUNS_PER_LEVEL as f64 / CONFIGURATION.RUNS_LAST_LEVEL as f64) as usize), new_capacity, level_len + 1));
                drop(levels);
                self.clear_prev_level(level_len);
            } else {
                drop(levels);
                self.flush_files(num_levels, level_files);
                levels = self.levels.read();
            }
        }
    }

    pub fn get(&self, key: &i32) -> Option<String> {
        // repeated code, not sure how to factor out
        // check buffer first 
        let buffer = self.buffer.read();
        match buffer.get(key) {
            Some(value) => {
                if value == &"0".repeat(CONFIGURATION.RECORD_SIZE - CONFIGURATION.KEY_SIZE) {
                    return None;
                }
                return Some(value.to_string());
            },
            None => {
                drop(buffer);
                let levels = self.levels.read();
                // now check disk levels
                for level in levels.iter() {
                    match level.get(key) {
                        Some(value) => {
                            if value == "0".repeat(CONFIGURATION.RECORD_SIZE - CONFIGURATION.KEY_SIZE) {
                                return None;
                            }
                            return Some(value);
                        },
                        None => (),
                    }
                }
                return None;
            }
        }
    }

    pub fn find_range(&self, lower: &i32, upper: &i32) -> Vec<Record> {
        let mut ranges = Vec::new();
        // MUST UPDATE!!!
        let levels = self.levels.read();
        for level in levels.iter() {
            ranges.push(level.find_range(lower, upper));
        }
        merge_k_sorted(ranges)
    }

    pub fn delete(&self, key: &i32) {
        let buffer = self.buffer.read();
        if buffer.is_full() && buffer.get(key).is_some() {
            drop(buffer);
            self.compaction();
        } else {
            drop(buffer);
        }
        let mut buffer = self.buffer.write();
        buffer.delete(key);
    }

    pub fn print_stats(&self) {    
        let levels = self.levels.read();
        debug!("LOGICAL PAIRS: will be printed at end");
        let mut distinct_keys = HashSet::new();
        for level in levels.iter() {
            print!("LVL{}: {}", level.level(), level.size() / CONFIGURATION.RECORD_SIZE);
        }
        for level in levels.iter() {
            level.print_stats(&mut distinct_keys);
        }
        debug!("LOGICAL PAIRS: {}", distinct_keys.len());
    }

    pub fn delete_files(&self) {
        let levels = self.levels.read();
        for level in levels.iter() {
            level.clear();
        }
    }
}
