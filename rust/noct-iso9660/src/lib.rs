#![no_std]

extern crate alloc;

use core::ffi::{c_char, c_void};

use alloc::vec::Vec;
use iso9660_simple::{helpers::get_directory_entry_by_path, ISODirectoryEntry};
// use noct_dpm_sys::Disk;
use noct_fs_sys::{
    FSM_DIR, FSM_ENTITY_TYPE_TYPE_DIR, FSM_ENTITY_TYPE_TYPE_FILE, FSM_FILE, FSM_MOD_READ, FSM_TIME,
};
use noct_logger::{qemu_err, qemu_log, qemu_note};
use noct_tools::raw_ptr_to_str;

const ISO9660_OEM: [u8; 5] = [67, 68, 48, 48, 49];
static FSNAME: &[u8] = b"ISO9660\0";

struct ThatDisk<'disk> {
    disk_name: &'disk str
}

impl iso9660_simple::Read for ThatDisk<'_> {
    fn read(&mut self, position: usize, buffer: &mut [u8]) -> Option<()> {
        noct_diskman::read(&self.disk_name, position as _, buffer);

        Some(())
    }
}

#[inline]
fn iso_type_to_fsm_type(entry: &ISODirectoryEntry) -> u32 {
    if entry.is_folder() {
        FSM_ENTITY_TYPE_TYPE_DIR
    } else {
        FSM_ENTITY_TYPE_TYPE_FILE
    }
}

#[inline]
fn iso_datetime_to_fsm(entry: &ISODirectoryEntry) -> FSM_TIME {
    FSM_TIME {
        year: 1900 + entry.record.datetime.year as u16,
        month: entry.record.datetime.month as _,
        day: entry.record.datetime.day as _,
        hour: entry.record.datetime.hour as _,
        minute: entry.record.datetime.minute as _,
        second: entry.record.datetime.second as _,
    }
}

unsafe extern "C" fn fun_read(
    disk_name: *const c_char,
    path: *const c_char,
    offset: u32,
    count: u32,
    buffer: *mut c_void,
) -> u32 {
    let mut fl = iso9660_simple::ISO9660::from_device(ThatDisk {
        disk_name: raw_ptr_to_str(disk_name),
    }).unwrap();

    let rpath = raw_ptr_to_str(path);

    let entries = match get_directory_entry_by_path(&mut fl, rpath) {
        Some(entry) => entry,
        None => return 0
    };

    let outbuf = core::slice::from_raw_parts_mut(buffer as *mut u8, count as _);

    let rd = noct_diskman::read(
        raw_ptr_to_str(disk_name),
        ((entries.lsb_position() * 2048) + offset) as u64,
        outbuf,
    );

    rd as _
}

unsafe extern "C" fn fun_write(_disk_name: *const c_char, _path: *const c_char, _c: u32, _d: u32, _e: *const c_void) -> u32 {
    qemu_err!("Writing is not supported!");
    0
}

unsafe extern "C" fn fun_info(disk_name: *const c_char, path: *const c_char) -> FSM_FILE {
    // let dev = noct_dpm_sys::get_disk(char::from_u32(letter as u32).unwrap()).unwrap();
    let mut fl = iso9660_simple::ISO9660::from_device(ThatDisk {
        disk_name: raw_ptr_to_str(disk_name),
    }).unwrap();

    let rpath = raw_ptr_to_str(path);

    let entry = get_directory_entry_by_path(&mut fl, rpath);

    if entry.is_none() {
        return FSM_FILE::missing();
    }

    let entry = entry.unwrap();
    let ftype = iso_type_to_fsm_type(&entry);

    FSM_FILE::with_data(
        &entry.name,
        0,
        entry.record.data_length.lsb,
        Some(iso_datetime_to_fsm(&entry)),
        ftype as _,
        FSM_MOD_READ,
    )
}

unsafe extern "C" fn fun_create(_disk_name: *const c_char, _b: *const c_char, _c: u32) -> i32 {
    qemu_err!("Creating is not supported!");
    0
}

unsafe extern "C" fn fun_delete(_disk_name: *const c_char, _b: *const c_char, _c: u32) -> i32 {
    qemu_err!("Deleting is not supported!");
    0
}

unsafe extern "C" fn fun_dir(disk_name: *const c_char, path: *const c_char, out: *mut FSM_DIR) {
    let mut fl = iso9660_simple::ISO9660::from_device(ThatDisk {
        disk_name: raw_ptr_to_str(disk_name),
    }).unwrap();

    let path = raw_ptr_to_str(path);

    qemu_note!("Path requested: {path}");

    let root = iso9660_simple::helpers::get_directory_entry_by_path(&mut fl, path);

    let root = match root {
        Some(root) => root,
        None => {
            *out = FSM_DIR::missing();
            return;
        }
    };

    let root = fl.read_directory(root.record.lba.lsb as _);

    let files: Vec<FSM_FILE> = root
        .map(|elem| {
            FSM_FILE::with_data(
                &elem.name,
                0,
                elem.record.data_length.lsb,
                Some(iso_datetime_to_fsm(&elem)),
                iso_type_to_fsm_type(&elem),
                FSM_MOD_READ,
            )
        })
        .collect();

    let files = files.into_boxed_slice();

    *out = FSM_DIR::with_files(files);
}

unsafe extern "C" fn fun_label(_disk_name: *const c_char, output: *mut c_char) {
    output.copy_from(FSNAME.as_ptr() as *const _, FSNAME.len());
}

unsafe extern "C" fn fun_detect(disk_name: *const c_char) -> i32 {
    let mut buffer = [0u8; 5];

    let disk_name = raw_ptr_to_str(disk_name);
    noct_diskman::read(disk_name, 0x8001, &mut buffer);

    if ISO9660_OEM != buffer {
        // qemu_err!(
        //     "Not valid ISO! (Disk: {disk_name:?})"            
        // );

        return 0;
    }

    qemu_log!(
        "Detected ISO! {disk_name:?}"
    );

    1
}

#[no_mangle]
pub extern "C" fn fs_iso9660_init() {
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
