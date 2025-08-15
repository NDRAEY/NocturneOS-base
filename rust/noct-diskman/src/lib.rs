#![no_std]

extern crate alloc;

pub mod c_api;
pub mod generic_drive;
pub mod partition;
pub mod structures;

use alloc::{
    borrow::ToOwned, boxed::Box, collections::linked_list::LinkedList, format, string::String,
};
use spin::RwLock;

use lazy_static::lazy_static;

use crate::structures::Drive;

lazy_static! {
    /// RwLock ensures nobody can edit LinkedList during disk operations
    /// (For example prevents disk removal [from software side] during operation)
    /// The second RwLock ensures that one operation at time per disk can be performed to avoid driver fooling and thus undefined behaviour.
    pub static ref DRIVES: RwLock<LinkedList<RwLock<Box<dyn Drive + Send + Sync + 'static>>>> =
        RwLock::new(LinkedList::new());
}

pub fn register_drive(drive: impl Drive + Send + Sync + 'static) {
    let mut a = DRIVES.write();

    a.push_back(RwLock::new(Box::new(drive)));
}

// pub fn unregister_drive(drive_id: &str) {
//     let lock = DRIVES.lock();
//     let mut binding = lock.borrow_mut();

//     let matched = binding.extract_if(|x| x.get_id() == drive_id);

//     matched.for_each(drop);
// }

pub fn generate_new_id(driver_id: &str) -> String {
    let last_number = DRIVES
        .read()
        .iter()
        .map(|x| x.read())
        .filter(|x| x.get_name().starts_with(driver_id))
        .map(|x| x.get_name()[driver_id.len()..].to_owned())
        .map(|x| x.parse::<usize>().unwrap_or(0))
        .last()
        .unwrap_or(0);

    format!("{driver_id}{last_number}")
}
