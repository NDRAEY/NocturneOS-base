#![no_std]
#![no_main]

extern crate alloc;

use core::panic::PanicInfo;

mod drv;
pub mod std;
mod system;

use alloc::vec;

#[panic_handler]
fn panic(_info: &PanicInfo) -> ! {
    println!("{}", _info);
    qemu_println!("{}", _info);
    loop {}
}

#[no_mangle]
#[inline(never)]
pub extern "C" fn rust_main() {
    let myvec = vec!["one", "two", "three"];

    println!("Привет, {}!", "Rust");

    for i in myvec {
        println!("{}", i);
    }
}
