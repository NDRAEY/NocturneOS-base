#![no_std]

extern crate alloc;

use alloc::{boxed::Box, format, vec::Vec};
use noct_dpm_sys::Disk;
use noct_fs_sys::{FSM_DIR, FSM_FILE, FSM_MOD_READ, FSM_TYPE_FILE, FSM_TYPE_DIR};
use noct_logger::{qemu_err, qemu_log, qemu_note, qemu_println};

const ISO9660_OEM: [u8; 5] = [67, 68, 48, 48, 49];
static FSNAME: &[u8] = b"LOLFS\0";

struct ThatDisk(Disk);

impl iso9660_simple::Read for ThatDisk {
    fn read(&mut self, position: usize, size: usize, buffer: &mut [u8]) -> Option<()> {
        self.0.read(0, position as u64, size, buffer);

        Some(())
    }
}

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
    qemu_err!("Writing is not supported!");
    0
}

unsafe extern "C" fn fun_info(_a: i8, _b: *const i8) -> FSM_FILE {
    qemu_log!("Info!");
    todo!()
}

unsafe extern "C" fn fun_create(_a: i8, _b: *const i8, _c: i32) -> i32 {
    qemu_log!("Creating is not supported!");
    0
}

unsafe extern "C" fn fun_delete(_a: i8, _b: *const i8, _c: i32) -> i32 {
    qemu_log!("Deleting is not supported!");
    0
}

unsafe extern "C" fn fun_dir(letter: i8, _b: *const i8, out: *mut FSM_DIR) {
    let dev = noct_dpm_sys::get_disk(char::from_u32(letter as u32).unwrap()).unwrap();

    let mut fl = iso9660_simple::ISO9660::from_device(ThatDisk(dev));
    let root = fl.read_root();

    core::sync::atomic::compiler_fence(core::sync::atomic::Ordering::SeqCst);
    for i in &root {
        core::sync::atomic::compiler_fence(core::sync::atomic::Ordering::SeqCst);
        qemu_println!("{}", i.name);
    }

    let files: Vec<FSM_FILE> = root.iter().map(|elem| {
        FSM_FILE::with_data(
            elem.name.clone(),
            0,
            elem.record.data_length.lsb,
            None,
            if elem.is_folder() {
                FSM_TYPE_DIR
            } else {
                FSM_TYPE_FILE
            } as _,
            FSM_MOD_READ,
        )
    }).collect();

    // let files = Box::new([
    //     FSM_FILE::with_data(
    //         "Ninjago lore.txt",
    //         0,
    //         1234,
    //         None,
    //         FSM_TYPE_FILE as _,
    //         FSM_MOD_READ,
    //     ),
    //     FSM_FILE::with_data(
    //         "Pokemon list.txt",
    //         0,
    //         1000,
    //         None,
    //         FSM_TYPE_FILE as _,
    //         FSM_MOD_READ,
    //     ),
    //     FSM_FILE::with_data(
    //         format!(
    //             "Youre listing !{}!.txt",
    //             char::from_u32(letter as u32).unwrap()
    //         ),
    //         0,
    //         5678,
    //         None,
    //         FSM_TYPE_FILE as _,
    //         FSM_MOD_READ,
    //     ),
    // ]);

    let files = files.into_boxed_slice();

    *out = FSM_DIR::with_files(files);
}

unsafe extern "C" fn fun_label(_a: i8, b: *mut i8) {
    b.copy_from(FSNAME.as_ptr() as *const _, 6);
    qemu_log!("Label!!");
}

unsafe extern "C" fn fun_detect(disk_letter: i8) -> i32 {
    let mut buffer = [0u8; 5];

    noct_dpm_sys::dpm_read(disk_letter, 0, 0x8001, 5, buffer.as_mut_ptr() as *mut _);

    if ISO9660_OEM != buffer {
        qemu_err!("Not valid ISO!");
        return 0;
    }

    qemu_log!(
        "Detected ISO! {}",
        char::from_u32(disk_letter as u32).unwrap()
    );

    1
}

pub fn fs_iso9660_init() {
    unsafe {
        noct_fs_sys::fsm_reg(
            FSNAME.as_ptr() as *const _,
            Some(fun_read),
            Some(fun_write),
            Some(fun_info),
            Some(fun_create),
            Some(fun_delete),
            Some(fun_dir),
            Some(fun_label),
            Some(fun_detect),
        )
    };

    unsafe { noct_fs_sys::fsm_dpm_update(-1) };
}
