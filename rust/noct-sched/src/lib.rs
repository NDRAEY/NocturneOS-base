#![no_std]
#![allow(non_upper_case_globals)]
#![allow(non_camel_case_types)]
#![allow(non_snake_case)]

use core::ffi::CStr;

use alloc::string::{String, ToString};

extern crate alloc;

include!(concat!(env!("OUT_DIR"), "/bindings.rs"));

impl process_t {
    pub fn cwd(&self) -> String {
        let c_str = unsafe { CStr::from_ptr(self.cwd as *const i8) };
        c_str.to_string_lossy().into_owned()
    }
}

#[inline]
pub fn me() -> &'static process_t {
    unsafe { &*get_current_proc() }
}

#[inline]
pub fn task_yield() {
    unsafe {
        task_switch_v2_wrapper(core::mem::zeroed::<registers>());
    };
}
