#![no_std]
#![no_main]

extern crate alloc;

use core::{arch::asm, cell::OnceCell, panic::PanicInfo};

pub mod audio;
pub mod drv;
pub mod gfx;
pub mod shell;
pub mod std;
pub mod system;
pub mod nd;

use noct_alloc::Allocator;
pub use noct_logger::*;

#[global_allocator]
static ALLOCATOR: Allocator = noct_alloc::Allocator;

#[panic_handler]
fn panic(_info: &PanicInfo) -> ! {
    qemu_err!("{}", _info);
    println!("{}", _info);

    unsafe {
        asm!("hlt");
    }
    
    loop {}
}

/// Main entry point for testing.
#[no_mangle]
#[inline(never)]
pub extern "C" fn rust_main() {
    println!("Привет, {}!", "Rust");

    // let mut p = Path::from_path("R:/").unwrap();
    // qemu_log!("{:?}", p);
    // qemu_log!("{:?}", p.apply(".."));

    // p.apply("1/2/../3/.././4/5/6"); // 1/4/5/6
    // qemu_log!("{:?}", p);

    // crate::std::thread::spawn(move || {
    //     for i in (1..=16) {
    //         qemu_ok!("{}", i);
    //     }
    // })

    noct_iso9660::fs_iso9660_init();
}
