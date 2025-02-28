use core::ops::Index;

use alloc::{str, string::String, vec::Vec};
use noct_logger::qemu_log;

use crate::FSM_FILE;

#[derive(Debug)]
pub struct File {
    file: FSM_FILE,
    pub name: String
}

impl File {
    pub fn from_fsm(f: &FSM_FILE) -> Self {
        let file = *f ;

        let lsi = file.Name.binary_search(&0).unwrap();

        qemu_log!("LSI: {}", lsi);

        let name_raw = &file.Name[..lsi];
        let prep: Vec<u8> = name_raw.iter().map(|a| *a as u8).collect();

        let name = String::from_utf8_lossy(prep.as_slice()).into_owned();

        File {
            file,
            name
        }
    }
}