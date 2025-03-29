#![no_std]

extern crate alloc;

use alloc::{string::{String, ToString}, vec::Vec};
use noct_fs_sys::{FSM_DIR, FSM_FILE, FSM_MOD_READ, FSM_TYPE_DIR, FSM_TYPE_FILE};
use noct_logger::{qemu_err, qemu_log, qemu_note};

static FSNAME: &[u8] = b"TARFS2\0";

pub mod disk_device;

impl tarfs::Device for disk_device::DiskDevice {}

fn raw_ptr_to_string(ptr: *const i8) -> String {
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
        tarfs::Type::Dir => FSM_TYPE_DIR,
        tarfs::Type::File => FSM_TYPE_FILE,
        _ => FSM_TYPE_FILE
    }
}

unsafe extern "C" fn fun_read(
    letter: i8,
    path: *const i8,
    offset: u32,
    count: u32,
    buffer: *mut i8,
) -> u32 {
    let dev = noct_dpm_sys::get_disk(char::from_u32(letter as u32).unwrap()).unwrap();
    let device = disk_device::DiskDevice::new(dev);

    let mut fl = tarfs::TarFS::from_device(device).unwrap();

    let path = ".".to_string() + &raw_ptr_to_string(path);
    let outbuf = core::slice::from_raw_parts_mut(buffer as *mut u8, count as _);

    let result = fl.read_file(&path, offset as _, count as _, outbuf);
    
    result.unwrap_or(0) as _
}

unsafe extern "C" fn fun_write(_a: i8, _b: *const i8, _c: u32, _d: u32, _e: *const i8) -> u32 {
    qemu_err!("Writing is not supported!");
    0
}

unsafe extern "C" fn fun_info(letter: i8, path: *const i8) -> FSM_FILE {
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

unsafe extern "C" fn fun_create(_a: i8, _b: *const i8, _c: i32) -> i32 {
    qemu_log!("Creating is not supported!");
    0
}

unsafe extern "C" fn fun_delete(_a: i8, _b: *const i8, _c: i32) -> i32 {
    qemu_log!("Deleting is not supported!");
    0
}

unsafe extern "C" fn fun_dir(letter: i8, path: *const i8, out: *mut FSM_DIR) {
    let dev = noct_dpm_sys::get_disk(char::from_u32(letter as u32).unwrap()).unwrap();
    let device = disk_device::DiskDevice::new(dev);

    let mut fl = tarfs::TarFS::from_device(device).unwrap();

    let path = ".".to_string() + &raw_ptr_to_string(path);

    qemu_note!("Path: {:?}", path);

    let data = fl.list_by_path_shallow(&path).unwrap();

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

unsafe extern "C" fn fun_label(_a: i8, b: *mut i8) {
    b.copy_from(FSNAME.as_ptr() as *const _, FSNAME.len());
}

unsafe extern "C" fn fun_detect(disk_letter: i8) -> i32 {
    let dev = noct_dpm_sys::get_disk(char::from_u32(disk_letter as u32).unwrap()).unwrap();
    let device = disk_device::DiskDevice::new(dev);

    let fl = tarfs::TarFS::from_device(device);

    if fl.is_some() { 1 } else { 0 }
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
