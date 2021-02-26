use std::cmp::Ordering;
use std::collections::BinaryHeap;
use log::{debug};
use std::sync::{Arc};
use std::time::{SystemTime};

use super::configuration::CONFIGURATION;
use super::lib_template::{Record};
use super::lib_helper::{generate_filename};
use super::lib_on_disk::lib_disk_file::{DiskFile};
use super::lib_on_disk::lib_disk_run::{Run};
use crate::metrics::{GET_IO_COUNTER_FOR_WRITES, PUT_IO_COUNTER};

use atomic_counter::AtomicCounter;

#[derive(Eq, Hash, PartialEq)]
pub struct HeapNode {
    element: Record, // the record stored in the heap node
    run_idx: usize, // index of run of element
    next_ele_idx: usize, // index of next elementin run
}

impl HeapNode {
    pub fn create_heap_node(element: Record, run_idx: usize, next_ele_idx: usize) -> HeapNode {
        HeapNode {
            element: element,
            run_idx: run_idx,
            next_ele_idx: next_ele_idx,
        }
    }
}

impl PartialOrd for HeapNode {
    fn partial_cmp(&self, other: &HeapNode) -> Option<Ordering> {
        Some(self.cmp(other))
    }
}

impl Ord for HeapNode {
    // flip orderings so priority queue becomes a min queue rather than a max queue
    fn cmp(&self, other: &HeapNode) -> Ordering {
        other.element.cmp(&self.element)
            // we want greater run indexes to have greater priority
            .then_with(|| self.run_idx.cmp(&other.run_idx))
    }
}

pub fn merge_from_files(mut files_to_merge: Vec<Vec<Arc<DiskFile>>>, run_merge_into: &Run, ts: SystemTime) -> Vec<Arc<DiskFile>> {
    let mut merged_runs: Vec<u8> = Vec::new();
    let mut merged_files: Vec<Arc<DiskFile>> = Vec::new();
    let mut heap = BinaryHeap::new();
    let mut runs_to_merge = Vec::new();
    for i in 0..files_to_merge.len() {
        let file_records = files_to_merge[i][0].read_all_file_records();
        GET_IO_COUNTER_FOR_WRITES.inc_by((files_to_merge[i][0].size as f64 / CONFIGURATION.BLOCK_SIZE as f64).ceil() as i64);
        PUT_IO_COUNTER.inc_by((files_to_merge[i][0].size as f64 / CONFIGURATION.BLOCK_SIZE as f64).ceil() as i64);
        let first = file_records[0].clone();
        files_to_merge[i].remove(0);
        runs_to_merge.push(file_records);
        heap.push(HeapNode::create_heap_node(first, i, 1));
    }
    let mut last_key = 0;
    while let Some(HeapNode {element, run_idx, mut next_ele_idx}) = heap.pop() {
        if merged_runs.len() == 0 || element.key != last_key {
            merged_runs.extend(&element.key.to_be_bytes());
            merged_runs.extend(element.value.as_bytes());
            last_key = element.key;
        }
        if next_ele_idx == runs_to_merge[run_idx].len() && files_to_merge[run_idx].len() > 0 {
            runs_to_merge[run_idx] = files_to_merge[run_idx][0].read_all_file_records();
            GET_IO_COUNTER_FOR_WRITES.inc_by((files_to_merge[run_idx][0].size as f64 / CONFIGURATION.BLOCK_SIZE as f64).ceil() as i64);
            PUT_IO_COUNTER.inc_by((files_to_merge[run_idx][0].size as f64 / CONFIGURATION.BLOCK_SIZE as f64).ceil() as i64);
            files_to_merge[run_idx].remove(0);
            next_ele_idx = 0;
        }
        if next_ele_idx < runs_to_merge[run_idx].len() {
            let next_element = runs_to_merge[run_idx][next_ele_idx].clone();
            assert!(next_element.key >= last_key);
            let new_node = HeapNode::create_heap_node(next_element, run_idx, next_ele_idx + 1);
            heap.push(new_node);
        } 
        if merged_runs.len() == CONFIGURATION.FILE_SIZE {
            let mut filename: &str = &format!("{}{}", CONFIGURATION.DB_PATH, generate_filename(run_merge_into.level, run_merge_into.run, run_merge_into.file_counter.get()));
            let merged_file = DiskFile::create_disk_file(filename.to_string(), &merged_runs, merged_runs.len(), ts);
            GET_IO_COUNTER_FOR_WRITES.inc_by((merged_file.size as f64 / CONFIGURATION.BLOCK_SIZE as f64).ceil() as i64);
            PUT_IO_COUNTER.inc_by((merged_file.size as f64 / CONFIGURATION.BLOCK_SIZE as f64).ceil() as i64);
            debug!("creating file {} in merge", &merged_file.filename.to_string());
            merged_files.push(Arc::new(merged_file));
            run_merge_into.file_counter.inc();
            merged_runs.clear();
        } 
    }
    if merged_runs.len() > 0 {
        let mut filename: &str = &format!("{}{}", CONFIGURATION.DB_PATH, generate_filename(run_merge_into.level, run_merge_into.run, run_merge_into.file_counter.get()));
        let merged_file = DiskFile::create_disk_file(filename.to_string(), &merged_runs, merged_runs.len(), ts);
        GET_IO_COUNTER_FOR_WRITES.inc_by((merged_file.size as f64 / CONFIGURATION.BLOCK_SIZE as f64).ceil() as i64);
        PUT_IO_COUNTER.inc_by((merged_file.size as f64 / CONFIGURATION.BLOCK_SIZE as f64).ceil() as i64);
        debug!("creating file {} in merge", &merged_file.filename);
        merged_files.push(Arc::new(merged_file));
        run_merge_into.file_counter.inc();
        merged_runs.clear();
    }
    merged_files
    //run_merge_into.insert_files(merged_files);
}

// pub fn merge_from_files(mut files_to_merge: Vec<Vec<Arc<DiskFile>>>, mut run_merge_into: &mut Run, ts: SystemTime) -> usize {
//     let mut total_size = 0;
//     let mut merged_runs: Vec<u8> = Vec::new();
//     let mut merged_files: Vec<Arc<DiskFile>> = Vec::new();
//     let mut last_merged_file = DiskFile::create_disk_file(generate_filename(run_merge_into.level, run_merge_into.run, run_merge_into.file_counter), &[], 0, ts);
//     run_merge_into.file_counter += 1;
//     let mut heap = BinaryHeap::new();
//     let mut runs_to_merge = Vec::new();
//     let mut files_positions = Vec::new();
//     for files in files_to_merge.iter_mut() {
//         runs_to_merge.push(files[0].read_file(0, CONFIGURATION.BLOCK_SIZE));
//         if files[0].size <= CONFIGURATION.BLOCK_SIZE {
//             files.remove(0);
//             files_positions.push(0);
//         } else {
//             files_positions.push(CONFIGURATION.BLOCK_SIZE);
//         }
//     }
//     for i in 0..runs_to_merge.len() {
//         heap.push(HeapNode::create_heap_node(runs_to_merge[i][0], i, 1)); // Store the first element of each vector
//     } 
//     let mut last_key = 0;
//     while let Some(HeapNode {element, run_idx, mut next_ele_idx}) = heap.pop() {
//         total_size += 1;
//         if merged_runs.len() == 0 || element.key != last_key {
//             merged_runs.extend(&element.key.to_be_bytes());
//             merged_runs.extend(&element.value.to_be_bytes());
//             last_key = element.key;
//         }
//         if next_ele_idx == runs_to_merge[run_idx].len() && files_to_merge[run_idx].len() > 0 {
//             let read_end = files_positions[run_idx] + CONFIGURATION.BLOCK_SIZE;
//             runs_to_merge[run_idx] = files_to_merge[run_idx][0].read_file(files_positions[run_idx], read_end);
//             if files_to_merge[run_idx][0].size <= read_end {
//                 files_to_merge[run_idx].remove(0);
//                 files_positions[run_idx] = 0;
//             } else {
//                 files_positions[run_idx] = read_end;
//             }
//             next_ele_idx = 0;
//         }
//         if next_ele_idx < runs_to_merge[run_idx].len() {
//             let next_element = runs_to_merge[run_idx][next_ele_idx];
//             assert!(next_element.key >= last_key);
//             let new_node = HeapNode::create_heap_node(next_element, run_idx, next_ele_idx + 1);
//             heap.push(new_node);
//         } 
//         if merged_runs.len() == CONFIGURATION.BLOCK_SIZE {
//             if last_merged_file.size == CONFIGURATION.FILE_SIZE {
//                 merged_files.push(Arc::new(last_merged_file));
//                 last_merged_file = DiskFile::create_disk_file(generate_filename(run_merge_into.level, run_merge_into.run, run_merge_into.file_counter), &merged_runs, merged_runs.len(), ts);
//                 run_merge_into.file_counter += 1;
//             } else {
//                 last_merged_file.write_file(&merged_runs, merged_runs.len());
//             }
//             merged_runs.clear();
//         } 
//     }
//     if last_merged_file.size == CONFIGURATION.FILE_SIZE {
        //     merged_files.push(Arc::new(last_merged_file));
        //     last_merged_file = DiskFile::create_disk_file(generate_filename(run_merge_into.level, run_merge_into.run, run_merge_into.file_counter), &merged_runs, merged_runs.len(), ts);
        //     run_merge_into.file_counter += 1;
        // } else {
        //     last_merged_file.write_file(&merged_runs, merged_runs.len());
        // }
//     merged_runs.clear();
//     run_merge_into.insert_files(merged_files);
//     total_size
// }

pub fn merge_k_sorted(vectors_to_merge: Vec<Vec<Record>>) -> Vec<Record> {
    let mut merged_vectors: Vec<Record> = Vec::new();
    let mut heap = BinaryHeap::new();
    for i in 0..vectors_to_merge.len() {
        heap.push(HeapNode::create_heap_node(vectors_to_merge[i][0].clone(), i, 1)); // Store the first element of each vector
    }
    let mut last_key = 0;
    while let Some(HeapNode {element, run_idx, next_ele_idx}) = heap.pop() {
        if merged_vectors.len() == 0 || element.key != last_key {
            merged_vectors.push(element.clone());
            last_key = element.key;
        }
        if next_ele_idx < vectors_to_merge[run_idx].len() {
            let next_element = vectors_to_merge[run_idx][next_ele_idx].clone();
            let new_node = HeapNode::create_heap_node(next_element, run_idx, next_ele_idx + 1);
            heap.push(new_node);
        }
    }
    merged_vectors
}

// merge runs is currently same as merge_k_sorted, except it writes the result
// into a byte vector rather than a vector of records
// we eventually want to change this, so it's not worth factoring out
pub fn merge_runs (runs_to_merge: Vec<Vec<Record>>) -> Vec<u8> {
    let mut merged_runs: Vec<u8> = Vec::new();
    let mut heap = BinaryHeap::new();
    for i in 0..runs_to_merge.len() {
        heap.push(HeapNode::create_heap_node(runs_to_merge[i][0].clone(), i, 1)); // Store the first element of each run
    }
    let mut last_key = 0;
    while let Some(HeapNode {element, run_idx, next_ele_idx}) = heap.pop() {
        if merged_runs.len() == 0 || element.key != last_key {
            merged_runs.extend(&element.key.to_be_bytes());
            merged_runs.extend(element.value.as_bytes());
            last_key = element.key;
        }
        if next_ele_idx < runs_to_merge[run_idx].len() {
            let next_element = runs_to_merge[run_idx][next_ele_idx].clone();
            let new_node = HeapNode::create_heap_node(next_element, run_idx, next_ele_idx + 1);
            heap.push(new_node);
        }
    }
    merged_runs
}