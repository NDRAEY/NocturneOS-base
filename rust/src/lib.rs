#![no_std]
#![no_main]

extern crate alloc;

use core::panic::PanicInfo;

pub mod audio;
pub mod drv;
pub mod gfx;
pub mod std;
pub mod system;

use alloc::{boxed::Box, string::{String, ToString}, vec};
use noct_alloc::Allocator;
use noct_fsm_sys::headers::{FSM_DIR, FSM_FILE, FSM_MOD_READ, FSM_TIME, FSM_TYPE_FILE};
use noct_logger::*;

#[global_allocator]
static ALLOCATOR: Allocator = noct_alloc::Allocator;

#[panic_handler]
fn panic(_info: &PanicInfo) -> ! {
    println!("{}", _info);
    qemu_println!("{}", _info);
    loop {}
}

const FSNAME: *const i8 = b"LOLFS\0".as_ptr() as _;

/// Main entry point for testing.
#[no_mangle]
#[inline(never)]
pub extern "C" fn rust_main() {
    println!("Привет, {}!", "Rust");

    unsafe {
        unsafe extern "C" fn fun_read(
            a: i8,
            b: *const i8,
            c: u32,
            d: u32,
            e: *mut core::ffi::c_void,
        ) -> u32 {
            qemu_log!("Read!");
            todo!()
        }

        unsafe extern "C" fn fun_write(
            a: i8,
            b: *const i8,
            c: u32,
            d: u32,
            e: *mut core::ffi::c_void,
        ) -> u32 {
            qemu_log!("Write!");
            todo!()
        }

        unsafe extern "C" fn fun_info(a: i8, b: *const i8) -> FSM_FILE {
            qemu_log!("Info!");
            todo!()
        }

        unsafe extern "C" fn fun_create(a: i8, b: *const i8, c: i32) -> i32 {
            qemu_log!("Create!");
            todo!()
        }

        unsafe extern "C" fn fun_delete(a: i8, b: *const i8, c: i32) -> i32 {
            qemu_log!("Delete!");
            todo!()
        }

        unsafe extern "C" fn fun_dir(a: i8, b: *const i8, out: *mut FSM_DIR) {
            let files = Box::new([FSM_FILE::with_data(
                "Ninjago lore.txt",
                0,
                1234,
                None,
                FSM_TYPE_FILE as _,
                FSM_MOD_READ,
            ),
            FSM_FILE::with_data(
                "Pokemon list.txt",
                0,
                1000,
                None,
                FSM_TYPE_FILE as _,
                FSM_MOD_READ,
            ),
            FSM_FILE::with_data(
                "WTF.txt",
                0,
                5678,
                None,
                FSM_TYPE_FILE as _,
                FSM_MOD_READ,
            )]);

            let dir = FSM_DIR::with_files(files);

            *out = dir;
        }

        unsafe extern "C" fn fun_label(a: i8, b: *mut i8) {
            b.copy_from(FSNAME, 6);
            qemu_log!("Label!!");
        }
        unsafe extern "C" fn fun_detect(a: i8) -> i32 {
            qemu_log!("Detect!");
            1
        }

        noct_fsm_sys::headers::fsm_reg(
            FSNAME,
            Some(fun_read),
            Some(fun_write),
            Some(fun_info),
            Some(fun_create),
            Some(fun_delete),
            Some(fun_dir),
            Some(fun_label),
            Some(fun_detect),
        );

        noct_fsm_sys::headers::fsm_dpm_update(-1);
    }

    // print!("Type> ");
    // let result = io::input::read_to_string();
    // println!("Result: '{}'", result);

    // crate::std::thread::spawn(move || {
    //     for i in (1..=16) {
    //         qemu_ok!("{}", i);
    //     }
    // })
}
