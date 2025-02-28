use alloc::string::{String, ToString};

use crate::{nvfs_close_dir, nvfs_dir};

#[derive(Debug)]
pub struct Directory {
    path: String,
    nvfs_ptr: *mut crate::FSM_DIR
}

impl Directory {
    pub fn from_path<PathPattern: ToString>(path: PathPattern) -> Self {
        let mut st = path.to_string();
        st.push('\0');

        let pr = unsafe { nvfs_dir(st.as_ptr() as *const _) };

        Self {
            path: st,
            nvfs_ptr: pr
        }
    }
}

impl Drop for Directory {
    fn drop(&mut self) {
        unsafe {
            nvfs_close_dir(self.nvfs_ptr);
        }
    }
}