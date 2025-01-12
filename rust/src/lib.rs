#![no_std]
#![no_main]

extern crate alloc;

use core::panic::PanicInfo;

pub mod drv;
pub mod gfx;
pub mod std;
pub mod system;

use std::io;

use alloc::{string::String, vec};

#[panic_handler]
fn panic(_info: &PanicInfo) -> ! {
    println!("{}", _info);
    qemu_println!("{}", _info);
    loop {}
}

/// Main entry point for testing.
#[no_mangle]
#[inline(never)]
pub extern "C" fn rust_main() {
    println!("Привет, {}!", "Rust");
}
