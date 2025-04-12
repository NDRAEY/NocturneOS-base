#![no_std]

extern crate alloc;

use alloc::{string::String, vec::Vec};
use disk_device::DiskDevice;
use noct_dpm_sys::Disk;
use noct_fs_sys::{FSM_DIR, FSM_FILE, FSM_MOD_READ, FSM_TYPE_DIR, FSM_TYPE_FILE, dir};
use noct_logger::{qemu_err, qemu_log, qemu_note, qemu_println};
use noctfs::{NoctFS, entity::Entity};

pub mod disk_device;

static FSNAME: &[u8] = b"NoctFS\0";

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

unsafe extern "C" fn fun_read(
    letter: i8,
    path: *const i8,
    offset: u32,
    count: u32,
    buffer: *mut i8,
) -> u32 {
    let disk = noct_dpm_sys::get_disk(char::from_u32(letter as u32).unwrap()).unwrap();

    let mut diskdev = DiskDevice::new(disk);
    let mut fs = noctfs::NoctFS::new(&mut diskdev).unwrap();

    let entity = find_by_path(&mut fs, raw_ptr_to_string(path)).unwrap();

    let outbuf = unsafe { core::slice::from_raw_parts_mut(buffer as *mut u8, count as _) };

    fs.read_contents_by_entity(&entity, outbuf, offset as _)
        .unwrap();

    count
}

unsafe extern "C" fn fun_write(_a: i8, _b: *const i8, _c: u32, _d: u32, _e: *const i8) -> u32 {
    todo!()
}

unsafe extern "C" fn fun_info(letter: i8, path: *const i8) -> FSM_FILE {
    let disk = noct_dpm_sys::get_disk(char::from_u32(letter as u32).unwrap()).unwrap();

    let mut diskdev = DiskDevice::new(disk);
    let mut fs = noctfs::NoctFS::new(&mut diskdev).unwrap();

    let entity = find_by_path(&mut fs, raw_ptr_to_string(path)).unwrap();

    qemu_note!("{entity:#x?}");

    let ftype = if entity.is_directory() {
        FSM_TYPE_DIR
    } else {
        FSM_TYPE_FILE
    };

    FSM_FILE::with_data(
        entity.name,
        0,
        entity.size as _,
        None,
        ftype as _,
        FSM_MOD_READ,
    )
}

unsafe extern "C" fn fun_create(_a: i8, _b: *const i8, _c: i32) -> i32 {
    todo!()
}

unsafe extern "C" fn fun_delete(_a: i8, _b: *const i8, _c: i32) -> i32 {
    todo!()
}

fn find_by_path(fs: &mut NoctFS<'_>, path: String) -> Option<Entity> {
    let mut initial = fs.get_root_entity().unwrap();

    let splitted = path.split("/").filter(|a| !a.is_empty());

    for i in splitted {
        let ents = fs.list_directory(initial.start_block);

        let mut found = false;

        for ent in ents {
            if ent.name == i {
                found = true;

                initial = ent;
                break;
            }
        }

        if !found {
            return None;
        }
    }

    Some(initial)
}

unsafe extern "C" fn fun_dir(letter: i8, _b: *const i8, out: *mut FSM_DIR) {
    let disk = noct_dpm_sys::get_disk(char::from_u32(letter as u32).unwrap()).unwrap();

    let mut diskdev = DiskDevice::new(disk);
    let mut fs = noctfs::NoctFS::new(&mut diskdev).unwrap();

    let directory_block = find_by_path(&mut fs, raw_ptr_to_string(_b));
    let entities = fs.list_directory(directory_block.unwrap().start_block);

    let files: Vec<FSM_FILE> = entities
        .iter()
        .map(|elem| {
            FSM_FILE::with_data(
                elem.name.clone(),
                0,
                elem.size as u32,
                None,
                if elem.is_directory() {
                    FSM_TYPE_DIR
                } else {
                    FSM_TYPE_FILE
                } as _,
                FSM_MOD_READ,
            )
        })
        .collect();

    let files = files.into_boxed_slice();

    unsafe { *out = FSM_DIR::with_files(files) };
}

unsafe extern "C" fn fun_label(_a: i8, b: *mut i8) {
    unsafe { b.copy_from(FSNAME.as_ptr() as *const _, FSNAME.len()) };
}

unsafe extern "C" fn fun_detect(disk_letter: i8) -> i32 {
    let dev = noct_dpm_sys::get_disk(char::from_u32(disk_letter as u32).unwrap()).unwrap();
    let mut device = disk_device::DiskDevice::new(dev);

    if noctfs::NoctFS::new(&mut device).is_err() {
        0
    } else {
        qemu_note!(
            "Detected NoctFS on: {}",
            char::from_u32(disk_letter as u32).unwrap()
        );
        1
    }
}

pub fn fs_noctfs_init() {
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
