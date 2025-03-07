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

use noct_alloc::Allocator;
pub use noct_logger::*;
use noct_zeraora::Heap;

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

static mut GLOBAL_KERNEL_HEAP: OnceCell<noct_zeraora::Heap> = OnceCell::new();

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

    // unsafe { GLOBAL_KERNEL_HEAP.set(Heap::with_entry_count(0x500_0000, 1024, 1024)) }.unwrap();
    // let mut heap: &mut Heap<'_> = unsafe { GLOBAL_KERNEL_HEAP.get_mut().unwrap() };
    
    // let block1 = heap.alloc_no_map(0x12, 1).unwrap();
    // let block2 = heap.alloc_no_map(6, 16).unwrap();

    // heap.free_no_unmap(block2).unwrap();

    // let block3 = heap.alloc_no_map(6, 16).unwrap();

    // qemu_note!("{:x} ({:x} = {:x})", block1, block2, block3);

    // let x = unsafe { GLOBAL_KERNEL_HEAP.get().unwrap() };

    noct_iso9660::fs_iso9660_init();
}
