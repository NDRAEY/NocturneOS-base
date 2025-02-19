#![no_std]
#![no_main]

extern crate alloc;

use core::panic::PanicInfo;

pub mod audio;
pub mod drv;
pub mod gfx;
pub mod std;
pub mod system;

use alloc::{string::String, vec};
use noct_alloc::Allocator;
use std::io;

#[global_allocator]
static ALLOCATOR: Allocator = noct_alloc::Allocator;

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

    // print!("Type> ");
    // let result = io::input::read_to_string();
    // println!("Result: '{}'", result);

    // crate::std::thread::spawn(move || {
    //     for i in (1..=16) {
    //         qemu_ok!("{}", i);
    //     }
    // })
}
