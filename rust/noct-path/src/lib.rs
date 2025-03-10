#![no_std]

extern crate alloc;

use alloc::{string::{String, ToString}, vec::Vec};

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

    pub fn from_path(path: &str) -> Option<Self> {
        Some(Path {
            buffer: path.to_string()
        })
    }

    fn sep(&self) -> Vec<&str> {
        let mut stems: Vec<&str> = Vec::new();

        for i in self.buffer.split('/') {
            if !i.is_empty() {
                stems.push(i);
            }
        }

        stems
    }

    pub fn apply(&mut self, path: &str) -> &mut Self {
        let mut stems: Vec<&str> = Vec::new();

        for i in path.split('/') {
            if !i.is_empty() {
                stems.push(i);
            }
        }

        for i in &stems {
            if i == &".." {
                let sep = self.sep();

                if sep.len() > 1 {
                    self.buffer = sep[..sep.len() - 1].join("/") + "/";
                }
            } else if i == &"." {
                // Do nothing
            } else {
                self.buffer += &(String::from(*i) + "/");
            }
        }

        self
    }

    pub fn parent(&mut self) -> &mut Self {
        // let stems: Vec<&str> = self.buffer.split("/").filter(|a| !a.is_empty()).collect();
        
        // IDK why it doesn't work, it gives random string array.
        let stems = self.sep();

        if stems.len() == 1 {
            self.buffer = stems.join("/") + "/";

            return self;
        }

        let mut finpath = stems[..stems.len() - 1].join("/");

        // If resulting path leads to root, we need to add some slash
        if finpath.len() == 2 {
            finpath += "/";
        }

        self.buffer = finpath;

        self
    }

    pub fn as_string(&self) -> &String {
        &self.buffer
    }

    pub fn as_str(&self) -> &str {
        &self.buffer
    }
}

impl ToString for Path {
    fn to_string(&self) -> String {
        self.buffer.clone()
    }
}