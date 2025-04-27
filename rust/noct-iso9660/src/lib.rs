#![no_std]

extern crate alloc;

use alloc::{string::String, vec::Vec};
use iso9660_simple::{helpers::get_directory_entry_by_path, ISODirectoryEntry};
use noct_dpm_sys::Disk;
use noct_fs_sys::{FSM_DIR, FSM_ENTITY_TYPE_TYPE_DIR, FSM_ENTITY_TYPE_TYPE_FILE, FSM_FILE, FSM_MOD_READ};
use noct_logger::{qemu_err, qemu_log, qemu_println};

const ISO9660_OEM: [u8; 5] = [67, 68, 48, 48, 49];
static FSNAME: &[u8] = b"ISO9660\0";

struct ThatDisk(Disk);

impl iso9660_simple::Read for ThatDisk {
    fn read(&mut self, position: usize, size: usize, buffer: &mut [u8]) -> Option<()> {
        self.0.read(0, position as u64, size, buffer);

        Some(())
    }
}

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

#[inline]
fn iso_type_to_fsm_type(entry: &ISODirectoryEntry) -> u32 {
    if entry.is_folder() {
        FSM_ENTITY_TYPE_TYPE_DIR
    } else {
        FSM_ENTITY_TYPE_TYPE_FILE
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
    let mut fl = iso9660_simple::ISO9660::from_device(ThatDisk(dev));

    let rpath = raw_ptr_to_string(path);

    let entry = get_directory_entry_by_path(&mut fl, &rpath);

    if entry.is_none() {
        return 0;
    }

    let entries = entry.unwrap();

    let outbuf = core::slice::from_raw_parts_mut(buffer as *mut u8, count as _);

    let dev = noct_dpm_sys::get_disk(char::from_u32(letter as u32).unwrap()).unwrap();
    let rd = dev.read(
        0,
        ((entries.record.lba.lsb * 2048) + offset) as u64,
        count as usize,
        outbuf,
    );

    rd as _
}

unsafe extern "C" fn fun_write(_a: i8, _b: *const i8, _c: u32, _d: u32, _e: *const i8) -> u32 {
    qemu_err!("Writing is not supported!");
    0
}

unsafe extern "C" fn fun_info(letter: i8, path: *const i8) -> FSM_FILE {
    let dev = noct_dpm_sys::get_disk(char::from_u32(letter as u32).unwrap()).unwrap();
    let mut fl = iso9660_simple::ISO9660::from_device(ThatDisk(dev));

    let rpath = raw_ptr_to_string(path);

    let entry = get_directory_entry_by_path(&mut fl, &rpath);

    if entry.is_none() {
        return FSM_FILE::missing();
    }

    let entry = entry.unwrap();
    let ftype = iso_type_to_fsm_type(&entry);

    FSM_FILE::with_data(
        entry.name,
        0,
        entry.record.data_length.lsb,
        None,
        ftype as _,
        FSM_MOD_READ,
    )
}

unsafe extern "C" fn fun_create(_a: i8, _b: *const i8, _c: u32) -> i32 {
    qemu_err!("Creating is not supported!");
    0
}

unsafe extern "C" fn fun_delete(_a: i8, _b: *const i8, _c: u32) -> i32 {
    qemu_err!("Deleting is not supported!");
    0
}

unsafe extern "C" fn fun_dir(letter: i8, _b: *const i8, out: *mut FSM_DIR) {
    let dev = noct_dpm_sys::get_disk(char::from_u32(letter as u32).unwrap()).unwrap();

    let mut fl = iso9660_simple::ISO9660::from_device(ThatDisk(dev));
    let root = fl.read_root();

    for i in &root {
        qemu_println!("{}", i.name);
    }

    let files: Vec<FSM_FILE> = root
        .iter()
        .map(|elem| {
            FSM_FILE::with_data(
                elem.name.clone(),
                0,
                elem.record.data_length.lsb,
                None,
                iso_type_to_fsm_type(&elem),
                FSM_MOD_READ,
            )
        })
        .collect();

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
        qemu_err!(
            "Not valid ISO! (Disk: {})",
            char::from_u32(disk_letter as u32).unwrap()
        );
        return 0;
    }

    qemu_log!(
        "Detected ISO! {}",
        char::from_u32(disk_letter as u32).unwrap()
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
