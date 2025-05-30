use alloc::{string::String, vec::Vec};

use crate::LoadInfo;

#[derive(Debug)]
pub struct ModuleHandle {
    path: String,

    entry_point: usize,
    loaded_segments: Vec<LoadInfo>,
}
