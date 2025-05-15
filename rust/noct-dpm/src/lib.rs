#![no_std]

extern crate alloc;

use core::ffi::CStr;

use alloc::string::String;
use noct_dpm_sys::{DPM_Disk, DPM_Disks};

pub struct Disk {
    pub letter: char,
    pub sector_size: usize,
    pub sector_count: u64,
    pub name: String,
    pub filesystem: String,
}

impl Disk {
    pub(crate) fn from_sys(letter: char, disk: &DPM_Disk) -> Option<Self> {
        if !disk.Ready {
            return None;
        }

        let name = unsafe { CStr::from_ptr(disk.Name) };
        let filesystem = unsafe { CStr::from_ptr(disk.FileSystem) };

        Some(Self {
            letter,
            sector_count: disk.Sectors as _,
            sector_size: disk.SectorSize as _,
            name: name.to_string_lossy().into_owned(),
            filesystem: filesystem.to_string_lossy().into_owned(),
        })
    }
}

const ALPHABET_SPAN: usize = 'Z' as usize - 'A' as usize;

pub struct Disks {
    index: usize,
}

impl Disks {
    pub fn new() -> Self {
        Self { index: 0 }
    }
}

#[inline(always)]
fn index_to_alphabet(index: u32) -> char {
    assert!(index <= ALPHABET_SPAN as u32);

    char::from_u32('A' as u32 + index).unwrap()
}

impl Iterator for Disks {
    type Item = Disk;

    fn next(&mut self) -> Option<Self::Item> {
        while self.index <= ALPHABET_SPAN {
            if unsafe { DPM_Disks[self.index].Ready } {
                break;
            }

            self.index += 1;
        }

        if self.index >= ALPHABET_SPAN {
            return None;
        }

        let disk = Disk::from_sys(
            index_to_alphabet(self.index as _),
            &unsafe { DPM_Disks }[self.index],
        );

        self.index += 1;

        disk
    }
}
