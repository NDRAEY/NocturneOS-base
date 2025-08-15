#![no_std]

extern crate alloc;

pub mod c_api;
pub mod generic_drive;
pub mod partition;
pub mod structures;

use alloc::{
    borrow::ToOwned, boxed::Box, collections::linked_list::LinkedList, format, string::String, vec,
};
use noct_logger::qemu_note;
use spin::RwLock;

use lazy_static::lazy_static;

use crate::structures::{Command, Drive};

lazy_static! {
    /// Drive storage
    ///
    /// RwLock ensures nobody can edit LinkedList during disk operations
    /// (For example prevents disk removal [from software side] during operation)
    /// The second RwLock ensures that one operation at time per disk can be performed to avoid driver fooling and thus undefined behaviour.
    pub static ref DRIVES: RwLock<LinkedList<RwLock<Box<dyn Drive + Send + Sync + 'static>>>> =
        RwLock::new(LinkedList::new());
}

pub fn register_drive(drive: impl Drive + Send + Sync + 'static) {
    // Add drive to the manager.
    
    {
        let mut lock = DRIVES.write();
        lock.push_back(RwLock::new(Box::new(drive)));
    }    

    // Run partition scanning

    {
        let id = {
            let lock = DRIVES.read();
            let disk_guard = lock.back().unwrap().read();

            disk_guard.get_id().to_owned()
        };

        rescan_disk_for_partitions(&id);
    }
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

/// Read data from disk.
/// 
/// Returns -1 if disk can't be found.
/// 
/// # Arguments:
/// 
/// * `disk_id` - ID of the target disk.
/// * `location` - Exact location in bytes.
/// * `buffer` - Output buffer where data is being written.
pub fn read(disk_id: &str, location: u64, buffer: &mut [u8]) -> i64 {
    let drives_handle = DRIVES.read();
    let disk_handle = drives_handle.iter().find(|x| x.read().get_id() == disk_id);

    match disk_handle {
        Some(x) => x.write().read(location, buffer),
        None => -1,
    }
}

/// Write data to disk.
/// 
/// Returns -1 if disk can't be found.
/// 
/// # Arguments:
/// 
/// * `disk_id` - ID of the target disk.
/// * `location` - Exact location in bytes.
/// * `buffer` - Output buffer where data is being copied from.
pub fn write(disk_id: &str, location: u64, buffer: &[u8]) -> i64 {
    let drives_handle = DRIVES.read();
    let disk_handle = drives_handle.iter().find(|x| x.read().get_id() == disk_id);

    match disk_handle {
        Some(x) => x.write().write(location, buffer),
        None => -1,
    }
}

/// Controls disk/drive.
/// 
/// Returns -1 if disk can't be found.
/// 
/// # Arguments:
/// 
/// * `command` - A command. Look [crate::structures::Command] for more.
/// * `command_parameters` - Some parameters for the command (may be empty).
/// * `data` - Output data buffer (may be empty).
pub fn control(disk_id: &str, command: Command, command_parameters: &[u8], data: &mut [u8]) -> i64 {
    let drives_handle = DRIVES.read();
    let disk_handle = drives_handle.iter().find(|x| x.read().get_id() == disk_id);

    match disk_handle {
        Some(x) => x.write().control(command, command_parameters, data),
        None => -1,
    }
}

pub fn rescan_disk_for_partitions(disk_id: &str) {
    let drives_handle = DRIVES.read();
    let disk_handle = drives_handle.iter().find(|x| x.read().get_id() == disk_id);

    qemu_note!("Disk: {disk_id:?}");

    match disk_handle {
        Some(x) => {
            let mut raw_buffer = vec![0u8; 512];

            x.write().read(0, &mut raw_buffer);

            let records = noct_mbr::parse_from_sector(&raw_buffer);

            for i in records {
                qemu_note!("{i:x?}");
            }
        },
        None => (),
    }
}