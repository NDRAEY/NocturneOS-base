#![no_std]
#![no_main]

extern crate alloc;

use core::{arch::asm, panic::PanicInfo};

pub mod audio;
pub mod drv;
pub mod gfx;
pub mod nd;
pub mod shell;
pub mod std;
pub mod system;

use noct_alloc::Allocator;
pub use noct_logger::*;
use zeraus::{
    components::{
        layout::linear::LinearLayout,
        rectangle::Rectangle,
    },
    draw::Draw,
};

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

    let mut lay = LinearLayout::new();
    let rect = Rectangle::new()
        .with_size(40, 40)
        .borders(0xff_ff0000, 5)
        .foreground_color(0xff_00ff00);

    let rect2 = Rectangle::new()
        .with_size(20, 20)
        .borders(0xff_ff00ff, 5)
        .foreground_color(0xff_0000ff);

    lay.push(rect);
    lay.push(rect2);

    let mut canvas = zeraus::canvas::Canvas::new(100, 100);
    lay.draw(&mut canvas, 0, 0);

    let (cw, ch) = (canvas.width(), canvas.height());

    for y in 0..ch {
        for x in 0..cw {
            let c = canvas.get_pixel(x, y).unwrap_or(0);
            let _ = noct_screen::set_pixel(x as u32, y as u32, c);
        }
    }

    noct_iso9660::fs_iso9660_init();
}
