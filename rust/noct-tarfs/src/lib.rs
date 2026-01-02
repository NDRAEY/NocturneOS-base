#![no_std]

extern crate alloc;

use core::ffi::{c_char, c_void};

use alloc::{string::ToString, vec::Vec};
use noct_fs_sys::{
    FSM_DIR, FSM_ENTITY_TYPE_TYPE_DIR, FSM_ENTITY_TYPE_TYPE_FILE, FSM_FILE, FSM_MOD_READ,
};
use noct_logger::{qemu_err, qemu_log};
use noct_tools::raw_ptr_to_str;

static FSNAME: &[u8] = b"TARFS2\0";

pub mod disk_device;

fn tarfs_type_to_fsm_type(tarfs_type: tarfs::Type) -> u32 {
    match tarfs_type {
        tarfs::Type::Dir => FSM_ENTITY_TYPE_TYPE_DIR,
        _ => FSM_ENTITY_TYPE_TYPE_FILE,
    }
}

unsafe extern "C" fn fun_read(
    disk_name: *const c_char,
    path: *const c_char,
    offset: u32,
    count: u32,
    buffer: *mut c_void,
) -> u32 {
    let device = disk_device::DiskDevice::new(disk_name);

    let mut fl = tarfs::TarFS::from_device(device).unwrap();

    let path = ".".to_string() + raw_ptr_to_str(path);
    let outbuf = core::slice::from_raw_parts_mut(buffer as *mut u8, count as _);

    let result = fl.read_file(&path, offset as _, &mut outbuf[..count as usize]);

    result.unwrap_or(0) as _
}

unsafe extern "C" fn fun_write(
    _disk_name: *const c_char,
    _b: *const c_char,
    _c: u32,
    _d: u32,
    _e: *const c_void,
) -> u32 {
    qemu_err!("Writing is not supported!");
    0
}

unsafe extern "C" fn fun_info(disk_name: *const c_char, path: *const c_char) -> FSM_FILE {
    let device = disk_device::DiskDevice::new(disk_name);

    let mut fl = tarfs::TarFS::from_device(device).unwrap();

    let path = ".".to_string() + raw_ptr_to_str(path);

    let entity = match fl.find_file(&path) {
        Err(_) => {
            qemu_err!("{path} is missing!");
            return FSM_FILE::missing()
        },
        Ok(e) => e,
    };

    let ftype = tarfs_type_to_fsm_type(entity._type);

    FSM_FILE::with_data(
        &entity.name,
        0,
        entity.size as _,
        None,
        ftype as _,
        FSM_MOD_READ,
    )
}

unsafe extern "C" fn fun_create(_disk_name: *const c_char, _b: *const c_char, _c: u32) -> i32 {
    qemu_log!("Creating is not supported!");
    0
}

unsafe extern "C" fn fun_delete(_disk_name: *const c_char, _b: *const c_char, _c: u32) -> i32 {
    qemu_log!("Deleting is not supported!");
    0
}

unsafe extern "C" fn fun_dir(disk_name: *const c_char, path: *const c_char, out: *mut FSM_DIR) {
    let device = disk_device::DiskDevice::new(disk_name);

    let mut fl = tarfs::TarFS::from_device(device).unwrap();

    let path = ".".to_string() + &raw_ptr_to_str(path);

    let data = fl.list_by_path_shallow(&path);

    if data.is_err() {
        *out = FSM_DIR::missing();
        return;
    }

    let data = data.unwrap();

    let files: Vec<FSM_FILE> = data
        .into_iter()
        .map(|elem| {
            FSM_FILE::with_data(
                &elem.name,
                0,
                elem.size as _,
                None,
                tarfs_type_to_fsm_type(elem._type) as _,
                FSM_MOD_READ,
            )
        })
        .collect();

    let files = files.into_boxed_slice();

    *out = FSM_DIR::with_files(files);
}

unsafe extern "C" fn fun_label(_disk_name: *const c_char, output_buffer: *mut c_char) {
    output_buffer.copy_from(FSNAME.as_ptr() as *const _, FSNAME.len());
}

unsafe extern "C" fn fun_detect(disk_name: *const c_char) -> i32 {
    let device = disk_device::DiskDevice::new(disk_name);

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
