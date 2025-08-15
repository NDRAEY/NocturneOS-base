#![no_std]

extern crate alloc;

pub mod c_api;
pub mod generic_drive;
pub mod structures;

use core::cell::RefCell;

use alloc::{boxed::Box, format, string::String, vec::Vec};
use spin::Mutex;

use crate::structures::Drive;

use lazy_static::lazy_static;

lazy_static! {
    pub static ref DRIVES: Mutex<RefCell<Vec<Box<dyn Drive + Send + 'static>>>> =
        Mutex::new(RefCell::new(Vec::new()));
}

pub fn diskman_register_drive(drive: impl Drive + Send + 'static) {
    let a = DRIVES.lock();

    a.borrow_mut().push(Box::new(drive));
}

pub fn generate_new_id(driver_id: &str) -> String {
    let last_number = DRIVES
        .lock()
        .borrow()
        .iter()
        .filter(|x| x.get_name().starts_with(driver_id))
        .map(|x| &x.get_name()[driver_id.bytes().len()..])
        .map(|x| x.parse::<usize>().unwrap_or(0))
        .last()
        .unwrap_or(0);

    format!("{driver_id}{last_number}")
}
