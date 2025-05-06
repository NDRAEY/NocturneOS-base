extern crate alloc;

use alloc::{boxed::Box, format, string::{String, ToString}};

use crate::{size_t, FSM_DIR, FSM_ENTITY_TYPE, FSM_ENTITY_TYPE_TYPE_DIR, FSM_ENTITY_TYPE_TYPE_FILE, FSM_FILE, FSM_TIME};

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
            if i.Type == FSM_ENTITY_TYPE_TYPE_FILE {
                files_c += 1;
            } else if i.Type == FSM_ENTITY_TYPE_TYPE_DIR  {
                dirs += 1;
            } else {
                other += 1;
            }
        }

        let ptr: &'static mut [FSM_FILE] = Box::leak(files);

        FSM_DIR {
            Ready: true,
            CountFiles: files_c,
            CountDir: dirs,
            CountOther: other,
            Files: ptr.as_mut_ptr(),
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
