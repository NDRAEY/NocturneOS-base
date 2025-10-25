#![no_std]
#![allow(non_upper_case_globals)]
#![allow(non_camel_case_types)]
#![allow(non_snake_case)]

include!(concat!(env!("OUT_DIR"), "/bindings.rs"));

extern crate alloc;

pub mod dir;
pub mod file;
pub mod impls;

// Under construction.

pub struct EntityInfo;
pub struct Directory;

#[repr(C)]
pub enum EntityType {
    File = 0,
    Folder = 1
    // ...
}


pub trait Filesystem {
    fn read(&mut self, file_path: &str, offset: u64, length: usize, buffer: &mut [u8]);
    fn write(&mut self, file_path: &str, offset: u64, length: usize, buffer: &[u8]);
    fn info(&self, file_path: &str) -> EntityInfo;
    fn dir(&mut self, file_path: &str) -> Directory;
    fn create(&mut self, file_path: &str, entity_type: EntityType) -> Directory;
    fn label(&self) -> &str;
    fn detect(disk_name: &str) -> Self;
}