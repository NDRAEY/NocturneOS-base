use core::ffi::{c_char, CStr};

use alloc::ffi::CString;

pub mod mem;
pub mod regs;
pub mod scheduler;
pub mod version;

unsafe extern "C" {
    fn chdir(dir: *const c_char);
}

pub fn chdir_nonrelative(dir: &str) {
    let s = CString::new(dir).unwrap();

    unsafe { chdir(s.as_ptr()) };
}