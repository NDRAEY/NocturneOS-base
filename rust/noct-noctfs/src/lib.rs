#![no_std]

extern crate alloc;

use core::ffi::{CStr, c_char, c_void};

use alloc::{
    string::{String, ToString},
    vec::Vec,
};
use disk_device::DiskDevice;
use noct_fs_sys::{
    FSM_DIR, FSM_ENTITY_TYPE_TYPE_DIR, FSM_ENTITY_TYPE_TYPE_FILE, FSM_FILE, FSM_MOD_READ,
};
use noct_logger::qemu_note;
use noctfs::{NoctFS, entity::Entity};

pub mod disk_device;

static FSNAME: &[u8] = b"NoctFS\0";

fn raw_ptr_to_str<'n>(ptr: *const c_char) -> &'n str {
    let c_str = unsafe { CStr::from_ptr(ptr) };
    c_str.to_str().unwrap()
}

unsafe extern "C" fn fun_read(
    disk_name: *const c_char,
    path: *const c_char,
    offset: u32,
    count: u32,
    buffer: *mut c_void,
) -> u32 {
    // let disk = noct_dpm_sys::get_disk(char::from_u32(letter as u32).unwrap()).unwrap();

    let mut diskdev = DiskDevice::new(disk_name);
    let mut fs = noctfs::NoctFS::new(&mut diskdev).unwrap();

    let entity = find_by_path(&mut fs, raw_ptr_to_str(path)).unwrap().1;

    let outbuf = unsafe { core::slice::from_raw_parts_mut(buffer as *mut u8, count as _) };

    fs.read_contents_by_entity(&entity, outbuf, offset as _)
        .unwrap();

    count
}

unsafe extern "C" fn fun_write(
    disk_name: *const c_char,
    path: *const c_char,
    offset: u32,
    count: u32,
    buffer: *const c_void,
) -> u32 {
    // let disk = noct_dpm_sys::get_disk(char::from_u32(letter as u32).unwrap()).unwrap();

    let mut diskdev = DiskDevice::new(disk_name);
    let mut fs = noctfs::NoctFS::new(&mut diskdev).unwrap();

    let (parent, entity) = find_by_path(&mut fs, raw_ptr_to_str(path)).unwrap();

    let outbuf = unsafe { core::slice::from_raw_parts(buffer as *const u8, count as _) };

    fs.write_contents_by_entity(parent.start_block, &entity, outbuf, offset as _)
        .unwrap();

    count
}

unsafe extern "C" fn fun_info(disk_name: *const c_char, path: *const c_char) -> FSM_FILE {
    // let disk = noct_dpm_sys::get_disk(char::from_u32(letter as u32).unwrap()).unwrap();

    let mut diskdev = DiskDevice::new(disk_name);
    let mut fs = noctfs::NoctFS::new(&mut diskdev).unwrap();

    let entity = find_by_path(&mut fs, raw_ptr_to_str(path));

    if entity.is_none() {
        return FSM_FILE::missing();
    }

    let entity = entity.unwrap().1;

    qemu_note!("{entity:#x?}");

    let ftype = if entity.is_directory() {
        FSM_ENTITY_TYPE_TYPE_DIR
    } else {
        FSM_ENTITY_TYPE_TYPE_FILE
    };

    FSM_FILE::with_data(
        &entity.name,
        0,
        entity.size as _,
        None,
        ftype as _,
        FSM_MOD_READ,
    )
}

unsafe extern "C" fn fun_create(disk_name: *const c_char, path: *const c_char, mode: u32) -> i32 {
    let path = raw_ptr_to_str(path);
    qemu_note!("Create: {}", &path);

    let mut diskdev = DiskDevice::new(disk_name);
    let mut fs = noctfs::NoctFS::new(&mut diskdev).unwrap();

    let (path, name) = {
        let mut rest_path = path
            .split('/')
            .filter(|a| !a.is_empty())
            .rev()
            .skip(1)
            .collect::<Vec<&str>>()
            .iter()
            .rev()
            .map(|&a| String::from(a))
            .collect::<Vec<_>>()
            .join("/");

        let name = path
            .split('/')
            .filter(|a| !a.is_empty())
            .next_back()
            .unwrap();

        if rest_path.is_empty() {
            rest_path = "/".to_string();
        }

        (rest_path, name)
    };

    qemu_note!("Path: {path:?}; Name: {name:?}");

    let directory_block = match find_by_path(&mut fs, &path).map(|a| a.1) {
        Some(blk) => blk,
        None => return 0,
    }
    .start_block;

    match mode {
        FSM_ENTITY_TYPE_TYPE_DIR => {
            fs.create_directory(directory_block, name);
        }
        FSM_ENTITY_TYPE_TYPE_FILE => {
            fs.create_file(directory_block, name);
        }
        _ => {
            panic!("create: Unknown type: {}", mode);
        }
    };

    1
}

unsafe extern "C" fn fun_delete(_disk_name: *const c_char, _b: *const c_char, _c: u32) -> i32 {
    todo!()
}

fn find_by_path(fs: &mut NoctFS<'_>, path: &str) -> Option<(Entity, Entity)> {
    let mut initial = fs.get_root_entity().unwrap();
    let mut previous = initial.clone();

    let splitted = path.split('/').filter(|a| !a.is_empty());

    for i in splitted {
        let ents = fs.list_directory(initial.start_block);

        let mut found = false;

        for ent in ents {
            if ent.name == i {
                found = true;

                previous = initial;
                initial = ent;
                break;
            }
        }

        if !found {
            return None;
        }
    }

    Some((previous, initial))
}

unsafe extern "C" fn fun_dir(disk_name: *const c_char, _b: *const c_char, out: *mut FSM_DIR) {
    // let disk = noct_dpm_sys::get_disk(char::from_u32(letter as u32).unwrap()).unwrap();

    let mut diskdev = DiskDevice::new(disk_name);
    let mut fs = noctfs::NoctFS::new(&mut diskdev).unwrap();

    let directory_block = find_by_path(&mut fs, raw_ptr_to_str(_b)).map(|a| a.1);
    let entities = fs.list_directory(directory_block.unwrap().start_block);

    let files: Vec<FSM_FILE> = entities
        .iter()
        .map(|elem| {
            FSM_FILE::with_data(
                &elem.name,
                0,
                elem.size as u32,
                None,
                if elem.is_directory() {
                    FSM_ENTITY_TYPE_TYPE_DIR
                } else {
                    FSM_ENTITY_TYPE_TYPE_FILE
                } as _,
                FSM_MOD_READ,
            )
        })
        .collect();

    let files = files.into_boxed_slice();

    unsafe { *out = FSM_DIR::with_files(files) };
}

unsafe extern "C" fn fun_label(_disk_name: *const c_char, output_buffer: *mut c_char) {
    unsafe { output_buffer.copy_from(FSNAME.as_ptr() as *const _, FSNAME.len()) };
}

unsafe extern "C" fn fun_detect(disk_name: *const c_char) -> i32 {
    // let dev = noct_dpm_sys::get_disk(char::from_u32(disk_letter as u32).unwrap()).unwrap();
    let mut device = disk_device::DiskDevice::new(disk_name);

    if noctfs::NoctFS::new(&mut device).is_err() {
        0
    } else {
        qemu_note!("Detected NoctFS on: {:?}", raw_ptr_to_str(disk_name));
        1
    }
}

#[unsafe(no_mangle)]
pub extern "C" fn fs_noctfs_init() {
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
