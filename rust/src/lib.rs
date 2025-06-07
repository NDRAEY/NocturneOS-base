#![no_std]
#![no_main]
#![allow(unused_imports)]
#![allow(static_mut_refs)]

extern crate alloc;

use core::{arch::asm, cell::OnceCell, panic::PanicInfo};

pub mod drv;
pub mod gfx;
pub mod shell;
pub mod std;
pub mod system;

use noct_elfloader::ko_modules::load_module;
use noct_il::log;
pub use noct_iso9660;
pub use noct_noctfs;
pub use noct_psf;
use noct_psf::PSF;
pub use noct_tarfs;

use noct_alloc::Allocator;
pub use noct_logger::*;
use noct_timer::{sleep_ms, timestamp};
use noct_tty::println;

pub use noct_audio::c_api;
pub use noct_ipc::manager::ipc_init;

pub use noct_fatfs;
#[cfg(target_arch = "x86")]
pub use noct_pci;
#[cfg(target_arch = "x86")]
pub use noct_ps2;

#[global_allocator]
static ALLOCATOR: Allocator = noct_alloc::Allocator;

unsafe extern "C" {
    fn unwind_stack(frames: u32);
}

#[panic_handler]
fn panic(_info: &PanicInfo) -> ! {
    qemu_err!("{}", _info);
    unsafe { unwind_stack(10) };

    println!("{}", _info);

    loop {
        #[cfg(target_arch = "x86")]
        unsafe {
            asm!("hlt");
        }
    }
}

/// Main entry point for testing.
#[no_mangle]
#[inline(never)]
pub extern "C" fn rust_main() {
    //load_module("E:/test_module.ko");

    // let mut p = Path::from_path("R:/").unwrap();
    // qemu_log!("{:?}", p);
    // qemu_log!("{:?}", p.apply(".."));

    // p.apply("1/2/../3/.././4/5/6"); // 1/4/5/6
    // qemu_log!("{:?}", p);

    // crate::std::thread::spawn(move || {
    //     for i in (1..=128) {
    //         qemu_ok!("{}", i);
    //     }
    // })

    // {
    //     unsafe { noct_sched::scheduler_mode(false) };

    //     noct_screen::fill(0);

    //     let screen_dimen = noct_screen::dimensions();
    //     let mut tty = noct_tty::console::Console::new(screen_dimen.1 / 16, screen_dimen.0 / 8);

    //     tty.print_str("Ninja-go!\n");
    //     tty.print_str("Hello World!\n");
    //     tty.print_str("\u{1b}[31mColorful text! (Red)\u{1b}[0m\n");
    //     tty.print_str("\u{1b}[32mColorful text! (Green)\u{1b}[0m\n");
    //     tty.print_str("\u{1b}[33mColorful text! (Yellow)\u{1b}[0m\n");
    //     tty.print_str("\u{1b}[34mColorful text! (Blue)\u{1b}[0m\n");
    //     tty.print_str("\u{1b}[35mColorful text! (Magenta)\u{1b}[0m\n");
    //     tty.print_str("\u{1b}[36mColorful text! (Cyan)\u{1b}[0m\n");
    //     tty.print_str("\u{1b}[37mColorful text! (White)\u{1b}[0m\n");

    //     tty.print_str("\u{1b}[2JWhoops!\n");

        // tty.print_str("Hello World!\n");
        // tty.print_str("\u{1b}[41mBackground color! (Red)\u{1b}[0m\n");
        // tty.print_str("\u{1b}[42mBackground color! (Green)\u{1b}[0m\n");
        // tty.print_str("\u{1b}[43mBackground color! (Yellow)\u{1b}[0m\n");
        // tty.print_str("\u{1b}[44mBackground color! (Blue)\u{1b}[0m\n");
        // tty.print_str("\u{1b}[45mBackground color! (Magenta)\u{1b}[0m\n");
        // tty.print_str("\u{1b}[46mBackground color! (Cyan)\u{1b}[0m\n");
        // tty.print_str("\u{1b}[47mBackground color! (White)\u{1b}[0m\n");

        // noct_screen::fill(0);
        // noct_tty::renderer::render(&mut tty);
        // noct_screen::flush();

        // loop {
        //     let start_time = timestamp();

        //     noct_screen::fill(0);
        //     noct_tty::renderer::render(&mut tty);
        //     noct_screen::flush();

        //     qemu_log!("Took: {} ms", timestamp() - start_time);
        // }
    // }

    // {
    //     let chan = create_named_channel("postman_guy").unwrap();

    //     chan.write("Hello!\0".as_bytes());

    //     spawn(move || {
    //         let my_chan = find_named_channel("postman_guy").unwrap();

    //         let mut data = [0u8; 6];

    //         my_chan.read(&mut data);

    //         qemu_ok!("Data is: {data:?}");
    //     });
    // }
}
