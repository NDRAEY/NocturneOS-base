// note: возможно лучшим решением будет переделать под определенные файловые системмы, но пока сойдет и так

#![no_std]
#![allow(non_upper_case_globals)]
#![allow(non_camel_case_types)]
#![allow(non_snake_case)]
include!(concat!(env!("OUT_DIR"), "/bindings.rs"));

extern crate alloc;

use alloc::ffi::CString;
use alloc::string::String;
use core::alloc::Layout;
use core::ffi::{CStr, c_void};

use alloc::vec::Vec;
use alloc::{str, vec};

unsafe impl Send for File {}

pub fn read_to_string(file_path: &str) -> Result<String, &str> {
    let cs = CString::new(file_path).unwrap();

    let file = unsafe { fopen(cs.as_ptr(), FileOpenMode_O_READ) };

    if file.is_null() {
        return Err("Failed to open file.");
    }

    let size = unsafe { fsize(file) as usize };
    
    let mut buffer: String = String::with_capacity(size);
    unsafe { buffer.as_mut_vec().set_len(size) };

    let ptr = buffer.as_mut_ptr() as *mut c_void;

    unsafe {
        fread(file, 1, size as _, ptr);

        fclose(file);
    }

    Ok(buffer)
}

pub fn read(file_path: &str) -> Result<Vec<u8>, &'static str> {
    let cs = CString::new(file_path).unwrap();

    let file = unsafe { fopen(cs.as_ptr(), FileOpenMode_O_READ) };

    if file.is_null() {
        return Err("Failed to open file.");
    }

    let size = unsafe { fsize(file) };
    let ptr = unsafe { alloc::alloc::alloc(Layout::array::<u8>(size as _).unwrap()) };

    unsafe {
        fread(file, size as _, 1, ptr as *mut _);

        fclose(file);
    }

    Ok(unsafe { Vec::from_raw_parts(ptr, size as _, size as _) })
}

pub fn write(file_path: &str, data: &[u8]) -> Result<usize, &'static str> {
    let cs = CString::new(file_path).unwrap();

    let file = unsafe { fopen(cs.as_ptr(), FileOpenMode_O_WRITE) };

    if file.is_null() {
        return Err("Failed to open file.");
    }

    unsafe {
        let written = fwrite(file, data.len() as _, 1, data.as_ptr() as *const _);

        fclose(file);

        Ok(written as usize)
    }
}

pub struct File {
    raw_file: &'static mut FILE,
    // path: String,
}

unsafe impl Sync for File {}

impl File {
    pub fn open(path: &str) -> Result<File, ()> {
        let cs = CString::new(path).unwrap();

        let file = unsafe { fopen(cs.as_ptr(), FileOpenMode_O_READ) };

        if file.is_null() {
            return Err(());
        }

        Ok(File {
            raw_file: unsafe { &mut *file },
            // path: String::from(path),
        })
    }

    pub fn read(&mut self, buf: &mut [u8]) -> Result<usize, &str> {
        let size = buf.len();
        let ptr = buf.as_mut_ptr() as *mut c_void;
        let data = unsafe { fread(self.raw_file, 1, size as _, ptr) };

        // if (data as usize) != size {
        //     return Err("Not enough data received");
        // }

        Ok(data as _)
    }

    pub fn write(&mut self, buf: &[u8]) -> Result<(), &str> {
        let size = buf.len();
        let ptr = buf.as_ptr() as *const c_void;
        let data = unsafe { fwrite(self.raw_file, 1, size as _, ptr) };

        if (data as usize) != size {
            return Err("Not enough data received");
        }

        Ok(())
    }

    pub fn read_to_string(&mut self, buf: &mut String) -> Result<(), &str> {
        let size = unsafe { fsize(self.raw_file) };
        let mut buffer: Vec<i8> = vec![0; size as usize]; // Создаем буфер для строки
        let ptr = buffer.as_mut_ptr() as *mut c_void;

        let data = unsafe { fread(self.raw_file, 1, size as _, ptr) };

        buffer.push(0);

        let raw = unsafe { CStr::from_ptr(buffer.as_mut_ptr() as *const _) };
        let s = raw.to_string_lossy();

        buf.push_str(&s);

        if data != size {
            return Err("Not enough data received");
        }

        Ok(())
    }

    pub fn rewind(&mut self) {
        unsafe { fseek(self.raw_file as *mut _, 0, 0) };
    }

    pub fn size(&mut self) -> usize {
        unsafe { fsize(self.raw_file) as _ }
    }
}

impl Drop for File {
    fn drop(&mut self) {
        unsafe { fclose(self.raw_file as *mut FILE) }
    }
}