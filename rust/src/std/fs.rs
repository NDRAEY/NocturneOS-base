// note: возможно лучшим решением будет переделать под определенные файловые системмы, но пока сойдет и так

use alloc::ffi::CString;
use alloc::string::String;
use core::ffi::{c_void, CStr};

use alloc::vec::Vec;
use alloc::{str, vec};

#[repr(C)]
struct CFile {
    path: *const i8,
    size: i32,
    fmode: u32,
    open: bool,
    pos: isize,
    err: u32,
}

extern "C" {
    fn fopen(filename: *const u8, mode: *const u8) -> *mut CFile;
    fn fclose(stream: *mut CFile);
    fn fsize(stream: *mut CFile) -> usize;
    fn fread(stream: *mut CFile, count: isize, size: usize, buffer: *mut c_void) -> i32;

    fn mkdir(path: *const i8) -> bool;
}

pub fn make_directory(path: &str) -> Result<(), &str> {
    let mut path_string = String::from(path);
    path_string.push('\0');

    let result = unsafe { mkdir(path_string.as_bytes().as_ptr() as *const _) };

    if result {
        Ok(())
    } else {
        Err("Failed to create directory.")
    }
}

pub fn read_to_string(file_path: &str) -> Result<&str, &str> {
    let mut file_path_string = String::from(file_path);
    file_path_string.push('\0');

    let file = unsafe { fopen(file_path_string.as_bytes().as_ptr(), c"r".as_ptr() as _) };

    if file.is_null() {
        return Err("Failed to open file.");
    }

    let size = unsafe { fsize(file) };
    let mut buffer: Vec<u8> = vec![0; size + 1]; // Создаем буфер для строки
    let ptr = buffer.as_mut_ptr() as *mut c_void;

    unsafe {
        fread(file, 1, size, ptr);

        fclose(file);
    }

    let result = unsafe {
        CStr::from_ptr(buffer.as_ptr() as *const i8)
            .to_str()
            .unwrap()
    };

    Ok(result)
}

pub struct File {
    raw_file: *mut CFile,
    // path: String,
}

impl File {
    pub fn open(path: &str) -> Result<File, ()> {
        let mut file_path_string = String::from(path);
        file_path_string.push('\0');

        let file = unsafe { fopen(file_path_string.as_bytes().as_ptr(), c"r".as_ptr() as _) };

        if file.is_null() {
            return Err(());
        }

        Ok(File {
            raw_file: file,
            // path: String::from(path),
        })
    }

    pub fn read(&mut self, buf: &mut [u8]) -> Result<(), &str> {
        let size = buf.len();
        let ptr = buf.as_mut_ptr() as *mut c_void;
        let data = unsafe { fread(self.raw_file, 1, size, ptr) };

        if (data as usize) != size {
            return Err("Not enough data received");
        }

        Ok(())
    }

    pub fn read_to_string(&mut self, buf: &mut String) -> Result<(), &str> {
        let size = unsafe { fsize(self.raw_file) };
        let mut buffer: Vec<i8> = vec![0; size]; // Создаем буфер для строки
        let ptr = buffer.as_mut_ptr() as *mut c_void;

        let data = unsafe { fread(self.raw_file, 1, size, ptr) };

        buffer.push(0);

        let raw = unsafe { CString::from_raw(buffer.as_mut_ptr()) };
        let s = raw.into_string().unwrap();

        buf.push_str(s.as_str());

        if (data as usize) != size {
            return Err("Not enough data received");
        }

        Ok(())
    }
}
