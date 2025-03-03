#![no_std]

extern crate alloc;

use alloc::boxed::Box;
use noct_fs_sys::{FSM_DIR, FSM_FILE, FSM_MOD_READ, FSM_TYPE_FILE};
use noct_logger::qemu_log;

static FSNAME: &[u8] = b"LOLFS\0";

unsafe extern "C" fn fun_read(
    _a: i8,
    _b: *const i8,
    _c: u32,
    _d: u32,
    _e: *mut core::ffi::c_void,
) -> u32 {
    qemu_log!("Read!");
    todo!()
}

unsafe extern "C" fn fun_write(
    _a: i8,
    _b: *const i8,
    _c: u32,
    _d: u32,
    _e: *const core::ffi::c_void,
) -> u32 {
    qemu_log!("Write!");
    todo!()
}

unsafe extern "C" fn fun_info(_a: i8, _b: *const i8) -> FSM_FILE {
    qemu_log!("Info!");
    todo!()
}

unsafe extern "C" fn fun_create(_a: i8, _b: *const i8, _c: i32) -> i32 {
    qemu_log!("Create!");
    todo!()
}

unsafe extern "C" fn fun_delete(_a: i8, _b: *const i8, _c: i32) -> i32 {
    qemu_log!("Delete!");
    todo!()
}

unsafe extern "C" fn fun_dir(_a: i8, _b: *const i8, out: *mut FSM_DIR) {
    let files = Box::new([
        FSM_FILE::with_data(
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
        FSM_FILE::with_data("WTF.txt", 0, 5678, None, FSM_TYPE_FILE as _, FSM_MOD_READ),
    ]);

    *out = FSM_DIR::with_files(files);
}

unsafe extern "C" fn fun_label(_a: i8, b: *mut i8) {
    b.copy_from(FSNAME.as_ptr() as *const _, 6);
    qemu_log!("Label!!");
}

unsafe extern "C" fn fun_detect(a: i8) -> i32 {
    qemu_log!("Detect! {}", char::from_u32(a as u32).unwrap());
    1
}

pub fn fs_iso9660_init() {
    unsafe { noct_fs_sys::fsm_reg(
        FSNAME.as_ptr() as *const _,
        Some(fun_read),
        Some(fun_write),
        Some(fun_info),
        Some(fun_create),
        Some(fun_delete),
        Some(fun_dir),
        Some(fun_label),
        Some(fun_detect),
    ) };

    unsafe { noct_fs_sys::fsm_dpm_update(-1) };
}
