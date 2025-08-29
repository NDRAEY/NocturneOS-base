#![no_std]
#![allow(non_upper_case_globals)]
#![allow(non_camel_case_types)]
#![allow(non_snake_case)]

extern crate alloc;

include!(concat!(env!("OUT_DIR"), "/bindings.rs"));

use core::{ffi::{c_char, CStr}, mem};
use alloc::{boxed::Box, ffi::CString, string::String, vec::Vec};

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

    let stems = name.split(":/").collect::<Vec<&str>>();

    if stems.len() < 2 {
        return Box::into_raw(info);
    }

    let disk_id = stems[0];
    let path = stems[1];

    // qemu_note!("DiskId: {disk_id:?}; Path: {path:?}");

    info.disk_id[..disk_id.len()].copy_from_slice(unsafe { mem::transmute(disk_id.as_bytes()) });
    
    info.Path = CString::new(String::from('/') + path).unwrap().into_raw();

    let disk_id_owned = CString::new(disk_id).unwrap();

    let filesystem_name = unsafe {
        fsm_get_disk_filesystem(disk_id_owned.as_ptr())
    };
    
	info.DriverFS = unsafe { fsm_getIDbyName(filesystem_name) };
    
    if info.DriverFS == -1 {
        return Box::into_raw(info);
    }

    info.Ready = true;

    Box::into_raw(info)
}
