extern crate alloc;

use alloc::{boxed::Box, format, string::{String, ToString}};
use core::ffi::c_int;

use crate::{size_t, FSM_DIR, FSM_ENTITY_TYPE, FSM_FILE, FSM_TIME};

impl FSM_FILE {
    pub fn with_data<T: ToString>(
        path: T,
        mode: core::ffi::c_int,
        size: size_t,
        time: Option<FSM_TIME>,
        typ: FSM_ENTITY_TYPE,
        access: u32,
    ) -> Self {
        let path = path.to_string();

        let mut splitted = path.split('/');

        let name: String = if path.ends_with('/') {
            let prelast = splitted.clone().count() - 2;

            splitted.nth(prelast).unwrap().to_string()
        } else {
            splitted.next_back().unwrap().to_string()
        };

        let max_len = name.len().min(1024);

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

        for (i, &byte) in name.as_bytes().iter().take(max_len).enumerate() {
            file.Name[i] = byte as i8;
        }

        for (i, &byte) in path.as_bytes().iter().take(max_len).enumerate() {
            file.Path[i] = byte as i8;
        }

        file
    }

    pub fn missing() -> Self {
        // `core::mem::zeroed`?
        FSM_FILE {
            Ready: false,
            Name: [0; 1024],
            Path: [0; 1024],
            Mode: 0,
            Size: 0,
            LastTime: unsafe { core::mem::zeroed() },
            Type: 0,
            CHMOD: 0,
        }
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

    pub fn missing() -> Self {
        // `core::mem::zeroed`?
        FSM_DIR {
            Ready: false,
            CountFiles: 0,
            CountDir: 0,
            CountOther: 0,
            Files: core::ptr::null_mut(),
        }
    }

}

impl FSM_TIME {
    pub fn format(&self) -> String {
        let yr = self.year;

        format!("{:02}.{:02}.{:04} {:02}:{:02}:{:02}", self.day, self.month, yr, self.hour, self.minute, self.second)
    }
}
