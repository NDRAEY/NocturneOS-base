#![no_std]

extern crate alloc;

use core::fmt::Display;

use alloc::{string::{String, ToString}, vec::Vec};

#[cfg(test)]
pub mod tests;

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
        if path.len() >= 3 {
            let letter = path.chars().nth(0).unwrap();
            let delim = &path[1..=2];

            if letter.is_alphabetic() && delim == ":/" {
                self.buffer = path.to_string();

                self.remove_trailing();
                
                return self;
            }
        }

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

        self.remove_trailing();

        self
    }

    fn remove_trailing(&mut self) {
        while self.buffer.ends_with('/') && (self.buffer.len() > 3) {
            self.buffer.pop();
        }
    }
    
    pub fn parent(&mut self) {
        // let stems: Vec<&str> = self.buffer.split("/").filter(|a| !a.is_empty()).collect();
        
        // IDK why it doesn't work, it gives random string array.
        let stems = self.sep();

        if stems.len() == 1 {
            self.buffer = stems.join("/") + "/";

            return;
        }

        let mut finpath = stems[..stems.len() - 1].join("/");

        // If resulting path leads to root, we need to add some slash
        if finpath.len() == 2 {
            finpath += "/";
        }

        self.buffer = finpath;
    }

    pub fn as_string(&self) -> &String {
        &self.buffer
    }

    pub fn as_str(&self) -> &str {
        &self.buffer
    }
}

impl Display for Path {
    fn fmt(&self, f: &mut core::fmt::Formatter<'_>) -> core::fmt::Result {
        f.write_str(&self.buffer)
    }
}
