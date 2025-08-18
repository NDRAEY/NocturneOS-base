#![no_std]

extern crate alloc;

use core::ffi::{c_char, c_void, CStr};

use alloc::{string::String, vec::Vec};
use fatfs::{FsOptions, Read, Seek, SeekFrom};
// use noct_dpm_sys::Disk;
use noct_fs_sys::{
    FSM_DIR, FSM_ENTITY_TYPE, FSM_ENTITY_TYPE_TYPE_DIR, FSM_ENTITY_TYPE_TYPE_FILE, FSM_FILE,
    FSM_MOD_READ, FSM_TIME,
};
use noct_logger::{qemu_err, qemu_note, qemu_ok};

static FSNAME: &[u8] = b"FATFS\0";

struct DiskFile {
    disk_name: String,
    position: u64,
}

impl fatfs::IoBase for DiskFile {
    type Error = ();
}

impl fatfs::Read for DiskFile {
    fn read(&mut self, buffer: &mut [u8]) -> Result<usize, ()> {
        let size = noct_diskman::read(&self.disk_name, self.position, buffer);

        if size != -1 {
            self.position += size as u64;
        }

        Ok(size as _)
    }
}

impl fatfs::Write for DiskFile {
    fn write(&mut self, buffer: &[u8]) -> Result<usize, Self::Error> {
        let size = noct_diskman::write(&self.disk_name, self.position, buffer);

        if size != -1 {
            self.position += size as u64;
        }

        Ok(size as _)
    }

    fn flush(&mut self) -> Result<(), Self::Error> {
        Ok(())
    }
}

impl fatfs::Seek for DiskFile {
    fn seek(&mut self, pos: SeekFrom) -> Result<u64, Self::Error> {
        match pos {
            SeekFrom::Start(p) => {
                self.position = p;
            }
            SeekFrom::End(_p) => {
                todo!("Check fatfs::SeekFrom::End!");
                // self.position = (self.disk.capacity() as i64 - p) as u64;
            }
            SeekFrom::Current(p) => {
                self.position = (self.position as i64 + p) as u64;
            }
        };

        Ok(self.position)
    }
}

fn raw_ptr_to_string(ptr: *const c_char) -> String {
    let c_str = unsafe { CStr::from_ptr(ptr) };
    c_str.to_string_lossy().into_owned()
}

unsafe extern "C" fn fun_read(
    disk_name: *const c_char,
    path: *const c_char,
    offset: u32,
    count: u32,
    buffer: *mut c_void,
) -> u32 {
    // let dev = noct_dpm_sys::get_disk(char::from_u32(letter as u32).unwrap()).unwrap();

    let fat = fatfs::FileSystem::new(
        DiskFile {
            disk_name: raw_ptr_to_string(disk_name),
            position: 0,
        },
        FsOptions::new(),
    )
    .unwrap();

    let path_binding = raw_ptr_to_string(path);
    let path = path_binding.trim();

    qemu_note!("Path: {path:?}");

    let mut file = match fat.root_dir().open_file(path) {
        Err(e) => {
            qemu_err!("Failed to read file: {:?}", e);
            return 0;
        }
        Ok(dir) => dir,
    };

    let filesize = file.seek(SeekFrom::End(0)).unwrap();
    file.seek(SeekFrom::Start(offset as u64)).unwrap();

    qemu_note!("File size is: {}", filesize);
    qemu_note!("Read at offset: {}; Read {} bytes", offset, count);

    let out_slice = unsafe { core::slice::from_raw_parts_mut(buffer as *mut u8, count as _) };

    file.read_exact(out_slice).unwrap();

    // qemu_note!("Read in: {} ms", timestamp() - stt);

    count
}

unsafe extern "C" fn fun_write(_disk_name: *const c_char, _path: *const c_char, _c: u32, _d: u32, _e: *const c_void) -> u32 {
    qemu_err!("Writing is not supported!");
    0
}

unsafe extern "C" fn fun_info(disk_name: *const c_char, path: *const c_char) -> FSM_FILE {
    // let dev = noct_dpm_sys::get_disk(char::from_u32(letter as u32).unwrap()).unwrap();

    let fat = fatfs::FileSystem::new(
        DiskFile {
            disk_name: raw_ptr_to_string(disk_name),
            position: 0,
        },
        FsOptions::new(),
    )
    .unwrap();

    let path_binding = raw_ptr_to_string(path);
    let path = path_binding.trim();

    let (pardir, filename) = {
        let mut stems = path
            .split("/")
            .filter(|a| !a.is_empty())
            .collect::<Vec<&str>>();
        let filename = stems.pop().unwrap();

        if stems.is_empty() {
            (String::from("/"), filename)
        } else {
            (stems.join("/"), filename)
        }
    };

    qemu_note!("{path:?} | {pardir:?}");

    let dir = if pardir.as_str() == "/" {
        Ok(fat.root_dir())
    } else {
        fat.root_dir().open_dir(pardir.as_str())
    };

    let dir = match dir {
        Err(e) => {
            qemu_err!("Failed to read dir: {:?}", e);
            return FSM_FILE::missing();
        }
        Ok(dir) => dir,
    };

    let candidate = dir
        .iter()
        .filter(|a| a.as_ref().unwrap().file_name() == filename)
        .next();

    match candidate {
        Some(Ok(c)) => FSM_FILE::with_data(
            c.file_name(),
            0,
            c.len() as _,
            Some(fat_time_to_fsm(c.modified())),
            fat_elem_to_fsm(&c),
            FSM_MOD_READ,
        ),
        _ => FSM_FILE::missing(),
    }
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
    // let dev = noct_dpm_sys::get_disk(char::from_u32(letter as u32).unwrap()).unwrap();

    let fat = fatfs::FileSystem::new(
        DiskFile {
            disk_name: raw_ptr_to_string(disk_name),
            position: 0,
        },
        FsOptions::new(),
    )
    .unwrap();

    let path_binding = raw_ptr_to_string(path);
    let path = path_binding.trim();

    qemu_note!("Path: {path:?}");

    let dir = if path == "/" {
        Ok(fat.root_dir())
    } else {
        fat.root_dir().open_dir(&path)
    };

    let dir = match dir {
        Ok(dir) => dir,
        Err(_e) => {
            unsafe { *out = FSM_DIR::missing() };
            return;
        }
    };

    let data: Vec<FSM_FILE> = dir
        .iter()
        .map(|elem| {
            let elem = elem.unwrap();

            FSM_FILE::with_data(
                elem.file_name(),
                0,
                elem.len() as _,
                Some(fat_time_to_fsm(elem.modified())),
                fat_elem_to_fsm(&elem),
                FSM_MOD_READ,
            )
        })
        .collect();

    let files = data.into_boxed_slice();

    unsafe { *out = FSM_DIR::with_files(files) };
}

fn fat_elem_to_fsm(
    elem: &fatfs::DirEntry<'_, DiskFile, fatfs::NullTimeProvider, fatfs::LossyOemCpConverter>,
) -> FSM_ENTITY_TYPE {
    if elem.is_dir() {
        FSM_ENTITY_TYPE_TYPE_DIR
    } else if elem.is_file() {
        FSM_ENTITY_TYPE_TYPE_FILE
    } else {
        todo!(
            "fat_elem_to_fsm expected to see file or directiory. But I got a special entity (not a file and not a directory)"
        )
    }
}

fn fat_time_to_fsm(modified: fatfs::DateTime) -> FSM_TIME {
    FSM_TIME {
        year: modified.date.year,
        month: modified.date.month as _,
        day: modified.date.day as _,
        hour: modified.time.hour as _,
        minute: modified.time.min as _,
        second: modified.time.sec as _,
    }
}

unsafe extern "C" fn fun_label(_disk_name: *const c_char, b: *mut c_char) {
    unsafe { b.copy_from(FSNAME.as_ptr() as *const _, FSNAME.len()) };
    // qemu_log!("Label!!");
}

unsafe extern "C" fn fun_detect(disk_name: *const c_char) -> i32 {
    // let dev = noct_dpm_sys::get_disk(char::from_u32(letter as u32).unwrap()).unwrap();

    // qemu_note!("Fatfs try to detect!");

    let fat = fatfs::FileSystem::new(
        DiskFile {
            disk_name: raw_ptr_to_string(disk_name),
            position: 0,
        },
        FsOptions::new(),
    );

    if fat.is_ok() {
        qemu_ok!("Detected FATFS!");
        return 1;
    }

    // if let Err(e) = fat {
    //     qemu_err!("Can't detect FATFS! {:?}", e);
    // }

    0
}

#[unsafe(no_mangle)]
pub extern "C" fn fs_fatfs_init() {
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
