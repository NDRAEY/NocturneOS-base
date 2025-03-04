extern crate alloc;

use alloc::{boxed::Box, format, string::{String, ToString}};
use core::ffi::c_int;

use crate::{size_t, FSM_DIR, FSM_FILE, FSM_TIME, FSM_TYPE_DIR, FSM_TYPE_FILE};

impl FSM_FILE {
    pub fn with_data<T: ToString>(
        name: T,
        mode: core::ffi::c_int,
        size: size_t,
        time: Option<FSM_TIME>,
        typ: c_int,
        access: u32,
    ) -> Self {
        let name = name.to_string();
        let name_bytes = name.as_bytes();
        let max_len = name_bytes.len().min(1024);

        let mut file = FSM_FILE {
            Ready: true,
            Name: [0; 1024],
            Path: [0; 1024],
            Mode: mode,
            Size: size,
            LastTime: time.unwrap_or_else(|| unsafe { core::mem::zeroed() }),
            Type: typ,
            CHMOD: access,
        };

        for (i, &byte) in name_bytes.iter().take(max_len).enumerate() {
            file.Name[i] = byte as i8;
        }

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

        let ptr = Box::leak(files).as_ptr();

        FSM_DIR {
            Ready: true,
            CountFiles: files_c,
            CountDir: dirs,
            CountOther: other,
            Files: ptr as *mut FSM_FILE,
        }
    }
}

impl FSM_TIME {
    pub fn format(&self) -> String {
        let yr = self.year;

        format!("{}.{}.{} {}:{}:{}", self.day, self.month, yr, self.hour, self.minute, self.second)
    }
}