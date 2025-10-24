#![no_std]

extern crate alloc;

pub mod c_api;
pub mod generic_drive;
pub mod partition;
pub mod structures;

use alloc::{
    borrow::ToOwned, boxed::Box, collections::linked_list::LinkedList, format, string::String,
    vec::Vec,
};
use noct_mbr::{PartitionRecord, PartitionType};
use spin::RwLock;

use lazy_static::lazy_static;

use crate::{
    partition::Partition,
    structures::{Command, Drive, DriveType},
};

lazy_static! {
    /// Drive storage
    ///
    /// RwLock ensures nobody can edit LinkedList during disk operations
    /// (For example prevents disk removal [from software side] during operation)
    /// The second RwLock ensures that one operation at time per disk can be performed to avoid driver fooling and thus undefined behaviour.
    pub static ref DRIVES: RwLock<LinkedList<RwLock<Box<dyn Drive + Send + Sync + 'static>>>> =
        RwLock::new(LinkedList::new());
}

pub fn register_drive(drive: Box<dyn Drive + Send + Sync + 'static>) {
    // Add drive to the manager.

    {
        let mut lock = DRIVES.write();
        lock.push_back(RwLock::new(drive));
    }

    // Run partition scanning

    let id = {
        let lock = DRIVES.read();
        let disk_guard = lock.back().unwrap().read();

        disk_guard.get_id().to_owned()
    };

    rescan_disk_for_partitions(&id);
}

pub fn generate_new_id(driver_id: &str) -> String {
    let last_number = DRIVES
        .read()
        .iter()
        .map(|x| x.read())
        .filter(|x| x.get_id().starts_with(driver_id))
        .map(|x| x.get_id()[driver_id.len()..].to_owned())
        .map(|x| {
            let first_section = x.split('.').nth(0).unwrap();

            first_section.parse::<usize>().unwrap_or(0)
        })
        .map(|x| x + 1)
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

fn remove_partitions(disk_id: &str) {
    let mut drives_handle = DRIVES.write();

    drives_handle
        .extract_if(|x| {
            // qemu_note!("{disk_id} == {:?}", x.read().get_parent_disk_id());

            x.read().get_parent_disk_id() == Some(disk_id)
        })
        .for_each(drop);
}

fn scan_partitions(
    disk: &mut dyn Drive,
    block_size: usize,
    container_base: Option<u32>,
    ebr_location: Option<u32>,
) -> Vec<PartitionRecord> {
    let mut raw_buffer = [0u8; 512];
    let read_at = ebr_location.unwrap_or(0) as u64 * block_size as u64;

    disk.read(read_at, &mut raw_buffer);

    let mut records: Vec<PartitionRecord> = noct_mbr::parse_from_sector(&raw_buffer);
    let mut filtered_records = Vec::new();

    for record in records.iter_mut() {
        if record.partition_type == PartitionType::Free {
            continue;
        }

        if let Some(base) = container_base {
            record.start_sector_lba += base;
        }

        match record.partition_type {
            PartitionType::Extended | PartitionType::ExtendedLBA => {
                if container_base.is_none() {
                    // Primary - add and recurse
                    filtered_records.push(record.clone());
                    let new_base = Some(record.start_sector_lba);
                    let sub = scan_partitions(disk, block_size, new_base, new_base);
                    filtered_records.extend(sub);
                } else {
                    // Extended - just recurse
                    let next_ebr = Some(record.start_sector_lba);
                    let sub = scan_partitions(disk, block_size, container_base, next_ebr);
                    filtered_records.extend(sub);
                }
            }
            _ => {
                filtered_records.push(record.clone());
            }
        }
    }

    filtered_records
}

pub fn rescan_disk_for_partitions(disk_id: &str) {
    // qemu_note!("Disk count before removing partitions: {}", DRIVES.read().len());
    remove_partitions(disk_id);
    // qemu_note!("Disk count after removing partitions: {}", DRIVES.read().len());

    let drives_handle = DRIVES.read();
    let disk_handle = drives_handle.iter().find(|x| x.read().get_id() == disk_id);

    // qemu_note!("Disk: {disk_id:?}");

    let mut new_partitions: LinkedList<RwLock<Box<dyn Drive + Send + Sync + 'static>>> =
        LinkedList::new();

    match disk_handle {
        Some(x) => {
            let sector_size = x.write().get_block_size().unwrap();
            let partitions = scan_partitions(x.write().as_mut(), sector_size as _, None, None);

            for (n, i) in partitions.iter().enumerate() {
                // I thought we need to compute byte-position in MBR (EBR) by multiplying starting
                // LBA by sector size which is different for various drive types
                // (e.g. optical drives have 2048 / 2352 bytes per sector,
                //   and hard drive have constantly 512 bytes per sector),

                // But I found that in terms of MBR / EBR we should assume that sector size
                // should be 512 bytes long no matter what.

                let offset_start = i.start_sector_lba as u64 * 512;
                let offset_end = (i.start_sector_lba + i.num_sectors) as u64 * 512;

                let partition = Partition {
                    parent_disk_name: x.read().get_name().to_owned(),
                    parent_disk_id: x.read().get_id().to_owned(),
                    // parition_number: n + 1,
                    internal_id: format!("{}.{}", x.read().get_id().to_owned(), n + 1),
                    offset_start,
                    offset_end,
                };

                new_partitions.push_back(RwLock::new(Box::new(partition)));
            }
        }
        None => (),
    }

    drop(drives_handle);

    let mut drives_handle = DRIVES.write();

    drives_handle.extend(new_partitions);
}

pub struct DiskInfo {
    pub name: String,
    pub id: String,
    pub is_partition: bool,
    pub drive_type: DriveType,
    pub capacity: Option<u64>,
    pub block_size: Option<u32>,
}

pub fn disk_list() -> Vec<DiskInfo> {
    let lock = DRIVES.read();

    lock.iter()
        .map(|x| {
            let mut handle = x.write();

            let name = handle.get_name().to_owned();
            let id = handle.get_id().to_owned();
            let is_partition = handle.is_partition();

            DiskInfo {
                name,
                id,
                is_partition: is_partition,
                drive_type: handle.get_type(),
                capacity: handle.get_capacity(),
                block_size: handle.get_block_size(),
            }
        })
        .collect()
}
