#![no_std]

#![allow(non_upper_case_globals)]
#![allow(non_camel_case_types)]
#![allow(non_snake_case)]

use alloc::string::String;

include!(concat!(env!("OUT_DIR"), "/bindings.rs"));

extern crate alloc;

pub enum EntityType {
    Directory,
    File
}

pub fn delete_file(path: &str) -> Option<()> {
    let mut fpath = String::from(path);
    fpath.push('\0');

    let rpath = fpath.as_ptr();

    let result = unsafe {
        unlink(rpath as *const _)
    };

    if result { Some(()) } else { None }
}

pub fn create_new_file(path: &str) -> Option<()> {
    let mut fpath = String::from(path);
    fpath.push('\0');

    let rpath = fpath.as_ptr();

    let result = unsafe {
        touch(rpath as *const _)
    };

    if result { Some(()) } else { None }
}

pub fn create_new_directory(path: &str) -> Option<()> {
    let mut fpath = String::from(path);
    fpath.push('\0');

    let rpath = fpath.as_ptr();

    let result = unsafe {
        mkdir(rpath as *const _)
    };

    if result { Some(()) } else { None }
}

pub fn get_type(path: &str) -> Option<EntityType> {
    let mut fpath = String::from(path);
    fpath.push('\0');

    let rpath = fpath.as_ptr();

    let is_f = unsafe {
        is_file(rpath as *const _)
    };

    let is_d = unsafe {
        is_dir(rpath as *const _)
    };

    if is_f && !is_d {
        Some(EntityType::File)
    } else if !is_f && is_d {
        Some(EntityType::Directory)
    } else {
        None
    }
}

pub fn size(path: &str) -> usize {
    let mut fpath = String::from(path);
    fpath.push('\0');

    let rpath = fpath.as_ptr();

    let size = unsafe {
        filesize(rpath as *const _)
    };

    size as _
}