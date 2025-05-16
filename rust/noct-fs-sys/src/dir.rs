use alloc::string::{String, ToString};
use noct_logger::qemu_note;

use crate::{file::File, nvfs_close_dir_v2, nvfs_dir_v2, FSM_DIR, FSM_FILE};

#[derive(Debug)]
pub struct Directory<'a> {
    // path: String,
    files: &'a [FSM_FILE],
    nvfs_dir: crate::FSM_DIR,
}

impl Directory<'_> {
    pub fn from_path<PathPattern: ToString>(path: &PathPattern) -> Option<Self> {
        let mut st = path.to_string();
        st.push('\0');

        let mut data = unsafe { core::mem::zeroed::<FSM_DIR>() };

        // let pr = unsafe { nvfs_dir(st.as_ptr() as *const _) };
        unsafe { nvfs_dir_v2(st.as_ptr() as *const _, &mut data as *mut _) };

        if !data.Ready {
            return None;
        }

        let dirs = data.CountDir;
        let files = data.CountFiles;
        let others = data.CountOther;
        let overall = dirs + files + others;

        Some(Self {
            // path: st,
            files: unsafe { core::slice::from_raw_parts(data.Files, overall as _) },            
            nvfs_dir: data,
        })
    }

    pub const fn directory_count(&self) -> u32 {
        self.nvfs_dir.CountDir as _
    }

    pub const fn file_count(&self) -> u32 {
        self.nvfs_dir.CountFiles as _
    }

    pub const fn other_count(&self) -> u32 {
        self.nvfs_dir.CountOther as _
    }

    pub const fn all_count(&self) -> usize {
        self.files.len() as _
    }

    pub fn is_accessible<PathPattern: ToString>(path: &PathPattern) -> bool {
        let dir = Directory::from_path(path);
        
        dir.is_some()
    }
}

impl Drop for Directory<'_> {
    fn drop(&mut self) {
        unsafe {
            nvfs_close_dir_v2(&mut self.nvfs_dir as *mut _);
        }
    }
}

pub struct DirectoryIter<'dir> {
    dir: &'dir Directory<'dir>,
    index: usize,
}

impl<'a> DirectoryIter<'a> {
    pub fn names(self) -> impl Iterator<Item = String> + use<'a> {
        self.map(|x| x.name)
    }
}

impl Iterator for DirectoryIter<'_> {
    type Item = File;

    fn next(&mut self) -> Option<Self::Item> {
        if self.index >= self.dir.files.len() {
            return None;
        }

        let a = &self.dir.files[self.index];

        self.index += 1;

        Some(File::from_fsm(*a))
    }
}

impl<'a> IntoIterator for &'a Directory<'_> {
    type Item = File;
    type IntoIter = DirectoryIter<'a>;

    fn into_iter(self) -> Self::IntoIter {
        DirectoryIter {
            dir: self,
            index: 0,
        }
    }
}
