#![no_std]

extern crate alloc;

pub mod generic_drive;
pub mod structures;
pub mod c_api;

use core::cell::{OnceCell, RefCell};

use alloc::{boxed::Box, format, string::String, vec::Vec};
use spin::{Mutex, rwlock::RwLock};

use crate::structures::Drive;

use lazy_static::lazy_static;

lazy_static! {
    pub static ref DRIVES: Mutex<RefCell<Vec<Box<dyn Drive + Send + 'static>>>> =
        Mutex::new(RefCell::new(Vec::new()));
}

pub fn generate_new_id(drive_name: &str) -> String {
    let last_number = DRIVES
        .lock()
        .borrow()
        .iter()
        .filter(|x| x.get_name().starts_with(drive_name))
        .map(|x| &x.get_name()[drive_name.bytes().len()..])
        .map(|x| x.parse::<usize>().unwrap_or(0))
        .last()
        .unwrap_or(0);

    format!("{drive_name}{last_number}")
}
