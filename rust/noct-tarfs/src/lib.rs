#![no_std]

extern crate alloc;

use core::ffi::{c_char, c_void};

use alloc::{
    string::{String, ToString},
    vec::Vec,
};
use noct_fs_sys::{
    FSM_DIR, FSM_ENTITY_TYPE_TYPE_DIR, FSM_ENTITY_TYPE_TYPE_FILE, FSM_FILE, FSM_MOD_READ,
};
use noct_logger::{qemu_err, qemu_log};

static FSNAME: &[u8] = b"TARFS2\0";

pub mod disk_device;

impl tarfs::Device for disk_device::DiskDevice {}

fn raw_ptr_to_string(ptr: *const c_char) -> String {
    let rpath = unsafe {
        core::slice::from_raw_parts(ptr as *const u8, {
            let mut ln = 0;

            loop {
                let byte = ptr.add(ln).read_volatile();

                if byte == 0 {
                    break;
                }

                ln += 1;
            }

            ln
        })
    };

    String::from_utf8(rpath.to_vec()).unwrap()
}

fn tarfs_type_to_fsm_type(tarfs_type: tarfs::Type) -> u32 {
    match tarfs_type {
        tarfs::Type::Dir => FSM_ENTITY_TYPE_TYPE_DIR,
        _ => FSM_ENTITY_TYPE_TYPE_FILE,
    }
}

unsafe extern "C" fn fun_read(
    letter: c_char,
    path: *const c_char,
    offset: u32,
    count: u32,
    buffer: *mut c_void,
) -> u32 {
    let dev = noct_dpm_sys::get_disk(char::from_u32(letter as u32).unwrap()).unwrap();
    let device = disk_device::DiskDevice::new(dev);

    let mut fl = tarfs::TarFS::from_device(device).unwrap();

    let path = ".".to_string() + &raw_ptr_to_string(path);
    let outbuf = core::slice::from_raw_parts_mut(buffer as *mut u8, count as _);
    let result = fl.read_file(&path, offset as _, &mut outbuf[..count as usize]);

    result.unwrap_or(0) as _
}

unsafe extern "C" fn fun_write(_a: c_char, _b: *const c_char, _c: u32, _d: u32, _e: *const c_void) -> u32 {
    qemu_err!("Writing is not supported!");
    0
}

unsafe extern "C" fn fun_info(letter: c_char, path: *const c_char) -> FSM_FILE {
    // qemu_note!("INFO!!!!!");

    let dev = noct_dpm_sys::get_disk(char::from_u32(letter as u32).unwrap()).unwrap();
    let device = disk_device::DiskDevice::new(dev);

    let mut fl = tarfs::TarFS::from_device(device).unwrap();

    let path = ".".to_string() + &raw_ptr_to_string(path);

    let entry = fl.find_file(&path);

    if entry.is_err() {
        return FSM_FILE::missing();
    }

    let entry = entry.unwrap();

    let ftype = tarfs_type_to_fsm_type(entry._type);

    FSM_FILE::with_data(
        entry.name,
        0,
        entry.size as _,
        None,
        ftype as _,
        FSM_MOD_READ,
    )
}

unsafe extern "C" fn fun_create(_a: c_char, _b: *const c_char, _c: u32) -> i32 {
    qemu_log!("Creating is not supported!");
    0
}

unsafe extern "C" fn fun_delete(_a: c_char, _b: *const c_char, _c: u32) -> i32 {
    qemu_log!("Deleting is not supported!");
    0
}

unsafe extern "C" fn fun_dir(letter: c_char, path: *const c_char, out: *mut FSM_DIR) {
    let dev = noct_dpm_sys::get_disk(char::from_u32(letter as u32).unwrap()).unwrap();
    let device = disk_device::DiskDevice::new(dev);

    let mut fl = tarfs::TarFS::from_device(device).unwrap();

    let path = ".".to_string() + &raw_ptr_to_string(path);

    let data = fl.list_by_path_shallow(&path);

    if data.is_err() {
        *out = FSM_DIR::missing();
        return;
    }

    let data = data.unwrap();

    let files: Vec<FSM_FILE> = data
        .iter()
        .map(|elem| {
            FSM_FILE::with_data(
                elem.name.clone(),
                0,
                elem.size as _,
                None,
                tarfs_type_to_fsm_type(elem._type.clone()) as _,
                FSM_MOD_READ,
            )
        })
        .collect();

    let files = files.into_boxed_slice();

    *out = FSM_DIR::with_files(files);
}

unsafe extern "C" fn fun_label(_a: c_char, b: *mut c_char) {
    b.copy_from(FSNAME.as_ptr() as *const _, FSNAME.len());
}

unsafe extern "C" fn fun_detect(disk_letter: c_char) -> i32 {
    let dev = noct_dpm_sys::get_disk(char::from_u32(disk_letter as u32).unwrap()).unwrap();
    let device = disk_device::DiskDevice::new(dev);

    let fl = tarfs::TarFS::from_device(device);

    if fl.is_some() {
        1
    } else {
        0
    }
}

#[no_mangle]
pub extern "C" fn fs_tarfs_register() {
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
}
