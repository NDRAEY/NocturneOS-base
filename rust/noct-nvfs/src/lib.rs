#![no_std]
#![allow(non_upper_case_globals)]
#![allow(non_camel_case_types)]
#![allow(non_snake_case)]

extern crate alloc;

include!(concat!(env!("OUT_DIR"), "/bindings.rs"));

use alloc::{boxed::Box, ffi::CString, string::String};
use noct_logger::{qemu_err, qemu_note};
use core::{
    ffi::{CStr, c_char},
    mem,
};

#[unsafe(no_mangle)]
pub unsafe extern "C" fn nvfs_decode(name: *const c_char) -> *mut NVFS_DECINFO {
    let mut info = Box::new(unsafe { core::mem::zeroed::<NVFS_DECINFO>() });

    if name.is_null() {
        return Box::into_raw(info);
    }

    let name = unsafe {
        let cstr = CStr::from_ptr(name);
        cstr.to_str().unwrap()
    };

    // qemu_note!("Path: {owned_string:?}");

    let mut stems = name.split(":/");

    let Some(disk_id) = stems.next() else {
        return Box::into_raw(info);
    };

    let Some(path) = stems.next() else {
        return Box::into_raw(info);
    };

    // qemu_note!("DiskId: {disk_id:?}; Path: {path:?}");

    info.disk_id[..disk_id.len()].copy_from_slice(unsafe { mem::transmute(disk_id.as_bytes()) });

    info.Path = CString::new(String::from('/') + path).unwrap().into_raw();

    let disk_id_owned = CString::new(disk_id).unwrap();

    let filesystem_name = unsafe { fsm_get_disk_filesystem(disk_id_owned.as_ptr()) };

    if filesystem_name.is_null() {
        qemu_err!("Filesystem name is null!");
        return Box::into_raw(info);
    }

    let filesystem_id = unsafe { fsm_getIDbyName(filesystem_name) };

    qemu_note!("FS ID: {}", filesystem_id);
    
    info.DriverFS = filesystem_id;

    if info.DriverFS == -1 {
        return Box::into_raw(info);
    }

    info.Online = true;
    info.Ready = true;

    Box::into_raw(info)
}
