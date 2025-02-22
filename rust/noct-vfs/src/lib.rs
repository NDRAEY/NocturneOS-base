#![no_std]

use alloc::{string::String, vec::Vec};

extern crate alloc;

#[repr(C)]
#[derive(Debug)]
pub enum FilesystemError {
    InvalidRequest,
    NotEnoughSpace,
    NotImplemented,
}

pub type Result<T> = core::result::Result<T, FilesystemError>;

#[repr(C)]
pub enum DirectoryEntryType {
    File,
    Directory,
    Unknown,
}

#[repr(C)]
pub struct DirectoryEntry {
    name: String,
    size: usize,
    create_timestamp: Time,
    modify_timestamp: Time,
    access_timestamp: Time,
}

pub trait Filesystem
where
    Self: Sized,
{
    fn probe(&mut self) -> bool;
    fn open(&self, path: &str) -> Result<impl File>;
    fn create(&mut self, path: &str) -> Result<()>;
    fn delete(&mut self, path: &str) -> Result<()>;
    fn info(&self, path: &str) -> Result<DirectoryEntry>;
    fn list(&self, path: &str) -> Result<Vec<DirectoryEntry>>;
}

#[repr(C)]
#[derive(Debug)]
pub enum SeekPosition {
    Current(usize),
    Start(usize),
    End(usize),
}

#[repr(C)]
#[derive(Debug)]
pub struct Time {
    pub year: u16,
    pub month: u8,
    pub day: u8,
    pub hour: u8,
    pub minute: u8,
    pub second: u8,
}

pub const ACCESS_READ: u8 = 1 << 0;
pub const ACCESS_WRITE: u8 = 1 << 1;
pub const ACCESS_EXECUTE: u8 = 1 << 2;

pub trait File {
    fn read(&self, buffer: &mut [u8], count: usize, size: usize) -> Result<()>;
    fn read_all(&self, buffer: &mut [u8]) -> Result<()> {
        self.read(buffer, buffer.len(), 1)
    }

    fn write(&mut self, buffer: &[u8], count: usize, size: usize) -> Result<()>;

    fn seek(&mut self, position: SeekPosition);
    fn tell(&self) -> usize;
}
