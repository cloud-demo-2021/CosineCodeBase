use std::convert::TryInto;
use std::fs::{File};
use std::io::{SeekFrom};
use std::io::prelude::*;
use std::time::{SystemTime, UNIX_EPOCH};
use log::{debug, info};
use std::fs::OpenOptions;
//use positioned_io::WriteAt;
use std::io;
use std::os::unix::prelude::FileExt;

use crate::configuration::CONFIGURATION;
use crate::lib_helper::{bytes_to_records, binary_search_fp};
use crate::lib_template::{Record};

pub struct DiskFile {
    pub filename: String,
    //pub compacting: AtomicBool,
    pub size: usize,
    pub ts: SystemTime,
    pub fence_pointers: Vec<i32>,
}

impl DiskFile {

    pub fn create_disk_file(filename: String, data: &[u8], size: usize, ts: SystemTime) -> DiskFile {
        debug!("creating file {}", &filename);

        let mut fence_pointers: Vec<i32> = Vec::new();

        // initialize fence pointers
        for i in (0..size).step_by(CONFIGURATION.RECORD_SIZE) {
            // keep track of max key in file
            if (i % CONFIGURATION.BLOCK_SIZE) == 0 || i == size - CONFIGURATION.RECORD_SIZE {
            //if (i % CONFIGURATION.BLOCK_SIZE) == 0 {
                let key = i32::from_be_bytes(data[i..i + CONFIGURATION.KEY_SIZE].try_into().unwrap());
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
            fence_pointers: fence_pointers,
        }
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

    pub fn read_file_to_buffer(&self, start_offset: usize, buffer: &mut[u8]) -> usize {
        let mut f = File::open(&self.filename).expect(&format!("Failed to open file {} for disk read", &self.filename));
        f.seek(SeekFrom::Start(start_offset as u64)).expect("Failed to seek file");
        f.read(&mut buffer[..]).expect("Failed to read file")
    }

    pub fn write_to_file(&self, start_offset: usize, data: &[u8]) {
        let mut f = OpenOptions::new()
        .write(true)
        .open(&self.filename)
        .expect("Unable to open");
        let bytes_written = f.write_all_at(&data, start_offset.try_into().unwrap());
        
        //assert!(bytes_written == CONFIGURATION.BLOCK_SIZE);
    }
}