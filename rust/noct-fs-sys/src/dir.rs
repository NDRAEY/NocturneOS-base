use alloc::string::{String, ToString};

use crate::{file::File, nvfs_close_dir, nvfs_dir, FSM_FILE};

#[derive(Debug)]
pub struct Directory {
    path: String,
    nvfs_ptr: *mut crate::FSM_DIR,
}

impl Directory {
    pub fn from_path<PathPattern: ToString>(path: PathPattern) -> Option<Self> {
        let mut st = path.to_string();
        st.push('\0');

        let pr = unsafe { nvfs_dir(st.as_ptr() as *const _) };
        let data = unsafe { *pr };

        if data.Ready == false {
            return None;
        }

        Some(Self {
            path: st,
            nvfs_ptr: pr,
        })
    }
}

impl Drop for Directory {
    fn drop(&mut self) {
        unsafe {
            nvfs_close_dir(self.nvfs_ptr);
        }
    }
}

pub struct DirectoryIter {
    dir: Directory,
    index: usize,
}

impl Iterator for DirectoryIter {
    type Item = File;

    fn next(&mut self) -> Option<Self::Item> {
        let dir = unsafe { *self.dir.nvfs_ptr };
        let len = dir.CountDir + dir.CountFiles + dir.CountOther;

        let files = unsafe { core::slice::from_raw_parts(dir.Files, len as _) };

        if self.index >= files.len() {
            return None;
        }

        let a = files[self.index];

        self.index += 1;

        Some(File::from_fsm(a))
    }
}

impl IntoIterator for Directory {
    type Item = File;
    type IntoIter = DirectoryIter;

    fn into_iter(self) -> Self::IntoIter {
        DirectoryIter {
            dir: self,
            index: 0,
        }
    }
}
