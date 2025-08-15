use alloc::string::String;

use crate::{structures::Drive, DRIVES};

pub struct Partition {
    parent_disk_name: String,
    parent_disk_id: String,

    parition_number: usize,
    
    internal_id: String,

    /// Starting offset in bytes
    offset_start: u64,

    /// Ending offset in bytes
    offset_end: u64
}

impl Drive for Partition {
    fn get_name(&self) -> &str {
        &self.parent_disk_name
    }

    fn get_id(&self) -> &str {
        &self.internal_id
    }

    fn get_parent_disk_id(&self) -> Option<&str> {
        Some(&self.parent_disk_id)
    }

    fn read(&mut self, location: u64, buffer: &mut [u8]) -> i64 {
        let binding = DRIVES.read();
        let parent_drive = binding.iter().find(|x| x.read().get_id() == self.parent_disk_id).expect("bug: failed to find a parent disk");

        if location >= self.offset_end {
            return 0;
        }

        parent_drive.write().read(self.offset_start + location, buffer)
    }

    fn write(&mut self, location: u64, buffer: &[u8]) -> i64 {
        let binding = DRIVES.read();
        let parent_drive = binding.iter().find(|x| x.read().get_id() == self.parent_disk_id).expect("bug: failed to find a parent disk");

        if location >= self.offset_end {
            return 0;
        }

        parent_drive.write().write(self.offset_start + location, buffer)
    }

    fn control(&mut self, command: crate::structures::Command, command_parameters: &[u8], data: &mut [u8]) -> i64 {
        let binding = DRIVES.read();
        let parent_drive = binding.iter().find(|x| x.read().get_id() == self.parent_disk_id).expect("bug: failed to find a parent disk");

        parent_drive.write().control(command, command_parameters, data)
    }
}