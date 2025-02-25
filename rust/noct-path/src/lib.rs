#![no_std]

extern crate alloc;

use alloc::{string::{String, ToString}, vec::Vec};
use noct_logger::qemu_log;

#[derive(Debug, Clone)]
pub struct Path {
    buffer: String,
}

impl Path {
    pub fn with_letter(letter: char) -> Self {
        let mut buf = String::new();
        buf.push(letter);

        Path {
            buffer: buf + ":/",
        }
    }

    pub fn from_path(path: &str) -> Self {
        Path {
            buffer: path.to_string()
        }
    }

    // pub fn apply(&mut self, path: &str) {
        // ...
    // }

    pub fn parent(&mut self) -> &Self {
        // let stems: Vec<&str> = self.buffer.split("/").filter(|a| !a.is_empty()).collect();
        
        let mut stems: Vec<&str> = Vec::new();

        for i in self.buffer.split("/") {
            if !i.is_empty() {
                stems.push(i);
            }
        }

        if stems.len() == 1 {
            return self;
        }

        let finpath = stems[..stems.len() - 1].join("/");

        self.buffer = finpath;

        self
    }

    pub fn as_str(&self) -> &str {
        &self.buffer
    }
}
