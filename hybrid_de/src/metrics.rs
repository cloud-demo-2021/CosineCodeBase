use lazy_static::lazy_static;
use prometheus::{self, IntCounter, register_int_counter};

lazy_static! {
    pub static ref GET_IO_COUNTER: IntCounter =
        register_int_counter!("getIOs", "Number of get disk IOs (by page)").unwrap();
    pub static ref PUT_IO_COUNTER: IntCounter =
        register_int_counter!("putIOs", "Number of get disk IOs (by page)").unwrap();
    // pub static ref PC_LEVELS_COUNTER: IntCounter =
    //     register_int_counter!("pcLevels", "Number of times partial compaction called for level").unwrap();
}
