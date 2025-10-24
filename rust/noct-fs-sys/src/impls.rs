extern crate alloc;

use alloc::{boxed::Box, ffi::CString, format, string::String};

use crate::{
    size_t, FSM_DIR, FSM_ENTITY_TYPE, FSM_ENTITY_TYPE_TYPE_DIR, FSM_ENTITY_TYPE_TYPE_FILE,
    FSM_FILE, FSM_TIME,
};

impl FSM_FILE {
    pub fn with_data(
        path: &str,
        mode: core::ffi::c_int,
        size: size_t,
        time: Option<FSM_TIME>,
        typ: FSM_ENTITY_TYPE,
        access: u32,
    ) -> Self {
        let mut splitted = path.split('/');

        let name = if path.ends_with('/') {
            let prelast = splitted.clone().count() - 2;

            splitted.nth(prelast).unwrap()
        } else {
            splitted.next_back().unwrap()
        };

        let raw_name = CString::new(name).unwrap().into_raw();
        let raw_path = CString::new(path).unwrap().into_raw();

        FSM_FILE {
            Ready: true,
            Name: raw_name,
            Path: raw_path,
            Mode: mode,
            Size: size,
            LastTime: time.unwrap_or_else(|| unsafe { core::mem::zeroed() }),
            Type: typ,
            CHMOD: access,
        }
    }

    #[inline]
    pub const fn missing() -> Self {
        unsafe { core::mem::zeroed() }
    }
}

impl FSM_DIR {
    pub fn with_files(files: Box<[FSM_FILE]>) -> Self {
        let (mut files_c, mut dirs, mut other) = (0, 0, 0);

        for i in files.iter() {
            if i.Type == FSM_ENTITY_TYPE_TYPE_FILE {
                files_c += 1;
            } else if i.Type == FSM_ENTITY_TYPE_TYPE_DIR {
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

    #[inline]
    pub const fn missing() -> Self {
        unsafe { core::mem::zeroed() }
    }
}

impl FSM_TIME {
    pub fn format(&self) -> String {
        let yr = self.year;

        format!(
            "{:02}.{:02}.{:04} {:02}:{:02}:{:02}",
            self.day, self.month, yr, self.hour, self.minute, self.second
        )
    }
}
