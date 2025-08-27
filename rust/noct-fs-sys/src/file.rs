use core::ffi::CStr;

use alloc::string::String;

use crate::FSM_FILE;

#[derive(Debug)]
pub struct File {
    pub file: FSM_FILE,
    pub name: String,
}

impl File {
    pub fn from_fsm(file: FSM_FILE) -> Self {
        let name = {
            let a = unsafe { CStr::from_ptr(file.Name) };
            a.to_string_lossy()
        }
        .into_owned();

        File { file, name }
    }
}
