#![no_std]

extern crate alloc;

use alloc::{alloc::{GlobalAlloc, Layout}, borrow::ToOwned};
use core::ffi::c_void;

extern "C" {
    fn kmalloc_common(size: usize, align: usize) -> *mut c_void;
    fn kfree(ptr: *mut c_void);
}

pub struct Allocator;
unsafe impl GlobalAlloc for Allocator {
    unsafe fn alloc(&self, layout: Layout) -> *mut u8 {
        let ptr = kmalloc_common(layout.size(), layout.align());

        if ptr.is_null() {
            panic!("Failed to allocate memory");
        }

        ptr as *mut u8
    }

    unsafe fn dealloc(&self, ptr: *mut u8, _layout: Layout) {
        kfree(ptr as *mut c_void);
    }
}
