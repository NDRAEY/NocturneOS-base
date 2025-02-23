extern crate alloc;

use alloc::boxed::Box;
use core::ffi::{c_int, CStr};
use noct_logger::*;

use crate::headers::{size_t, FSM_DIR, FSM_FILE, FSM_TIME, FSM_TYPE_DIR, FSM_TYPE_FILE};

impl FSM_FILE {
    pub fn with_data(
        name: &str,
        mode: core::ffi::c_int,
        size: size_t,
        time: Option<FSM_TIME>,
        typ: c_int,
        access: u32,
    ) -> Self {
        let ptr = name.as_bytes().as_ptr() as *const i8;
        let length = name.len();

        let mut file = FSM_FILE {
            Ready: true,
            Name: [0; 1024],
            Path: [0; 1024],
            Mode: mode,
            Size: size,
            LastTime: if time.is_some() {
                time.unwrap()
            } else {
                let a = [0u8; core::mem::size_of::<FSM_TIME>()];

                unsafe { core::mem::transmute(a) }
            },
            Type: typ,
            CHMOD: access,
        };

        unsafe {
            file.Name.as_mut_ptr().copy_from_nonoverlapping(ptr, length);

            // file.Path[0] = b'/' as i8;
            // file.Path.as_mut_ptr().add(1).copy_from(ptr, length);
        };

        file
    }
}

impl FSM_DIR {
    pub fn with_files(files: Box<[FSM_FILE]>) -> Self {
        let (mut files_c, mut dirs, mut other) = (0, 0, 0);

        for i in files.iter() {
            match i.Type as u32 {
                FSM_TYPE_FILE => {
                    files_c += 1;
                }
                FSM_TYPE_DIR => {
                    dirs += 1;
                }
                _ => {
                    other += 1;
                }
            }
        }

        let boxed = Box::new(files);

        let ptr = Box::leak(boxed).as_ptr();

        let dir = FSM_DIR {
            Ready: true,
            CountFiles: files_c,
            CountDir: dirs,
            CountOther: other,
            Files: ptr as *mut FSM_FILE,
        };

        dir
    }
}
