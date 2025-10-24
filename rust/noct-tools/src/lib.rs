#![no_std]

extern crate alloc;

pub mod memory;

use core::ffi::{CStr, c_char};

use alloc::{borrow::ToOwned, string::String};

pub unsafe fn raw_ptr_to_str<'ptr>(ptr: *const c_char) -> &'ptr str {
    let c_str = unsafe { CStr::from_ptr(ptr) };
    c_str.to_str().unwrap()
}

pub unsafe fn raw_ptr_to_string(ptr: *const c_char) -> String {
    unsafe { raw_ptr_to_str(ptr).to_owned() }
}