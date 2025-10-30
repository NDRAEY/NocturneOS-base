#![no_std]
#![allow(non_upper_case_globals)]
#![allow(non_camel_case_types)]
#![allow(non_snake_case)]
#![allow(unsafe_op_in_unsafe_fn)]

use core::ffi::{CStr, c_void};

use alloc::{boxed::Box, ffi::CString, string::String, vec::Vec};

extern crate alloc;

include!(concat!(env!("OUT_DIR"), "/bindings.rs"));

impl process_t {
    pub fn cwd(&self) -> String {
        let c_str = unsafe { CStr::from_ptr(self.cwd as *const _) };
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
        task_switch_v2_wrapper(core::mem::zeroed::<registers_t>());
    };
}

pub fn spawn_prog_rust(path: &str, args: &[&str]) -> i32 {
    let argc = args.len();

    let cstrings = args
        .iter()
        .map(|&x| CString::new(x).unwrap())
        .collect::<Vec<_>>();
    let argv = cstrings.iter().map(|x| x.as_ptr()).collect::<Vec<_>>();

    let path = CString::new(path).unwrap();
    let name = path.as_ptr();

    unsafe { spawn_prog(name, argc as _, argv.as_ptr()) }
}

type BoxedFnOnce = Box<dyn FnOnce() + Send>;

extern "C" fn trampoline(f: *mut ()) {
    // # Safety: we must guarantee f is a Box<dyn FnOnce()>
    let closure: Box<BoxedFnOnce> = unsafe { Box::from_raw(f as *mut _) };
    closure();
}

pub fn spawn(f: impl FnOnce() + Send + 'static) -> *mut thread_t {
    let boxed: Box<BoxedFnOnce> = Box::new(Box::new(f));
    let raw = Box::into_raw(boxed) as *mut ();

    unsafe {
        let proc = get_current_proc();

        thread_create_arg1(
            proc,
            trampoline as usize as *mut c_void,
            128 << 10,
            true,
            raw as u32,
        )
    }
}
