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

#[no_mangle]
#[inline(never)]
pub extern "C" fn io_test() {
    let mut buf = String::new();

    let mut file = std::fs::File::open("R:\\Zeraora.txt").unwrap();
    file.read_to_string(&mut buf).unwrap();

    println!("Data: [{}]", buf);
}

#[no_mangle]
#[inline(never)]
pub extern "C" fn rust_main() {
    println!("Привет, {}!", "Rust");

    io_test();
}
