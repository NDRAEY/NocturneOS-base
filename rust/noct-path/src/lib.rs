#![cfg_attr(not(test), no_std)]

extern crate alloc;

use core::fmt::Display;

use alloc::{
    borrow::ToOwned, string::{String, ToString}, vec::Vec
};

use core::cmp::PartialEq;

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

        Path { buffer: buf + ":/" }
    }

    pub fn from_path(path: &str) -> Option<Self> {
        Some(Path {
            buffer: path.to_string(),
        })
    }

    pub fn is_absolute(path: &str) -> bool {
        let delimited = path.split(":/").collect::<Vec<_>>();

        let has_valid_delims = delimited.len() == 2;
        
        if has_valid_delims {
            // If disk name is not empty
            if !delimited[0].is_empty() {
                // And first character in disk name is alphanumeric.
                if delimited[0].chars().next().unwrap().is_alphanumeric() {
                    return true;
                }
            }
        }

        return false;
    }

    pub fn disk_name(&self) -> &str {
        &self.buffer.split(":/").next().unwrap()
    }

    pub fn path_part(&self) -> &str {
        let mut iter = self.buffer.split(":/");

        iter.nth(1).unwrap_or("")
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
        if Path::is_absolute(path) {
            self.buffer = path.to_string();

            self.remove_trailing();

            return self;
        }

        for i in path.split('/') {
            if i == "." {
                continue;
            } else if i == ".." {
                self.remove_trailing();

                self.parent();
            } else if i == "" {
                continue;
            } else {
                if !self.buffer.ends_with('/') {
                    self.buffer.push('/');
                }

                self.buffer.push_str(i);
                self.buffer.push('/');
            }
        }

        self.remove_trailing();

        self
    }

    fn remove_trailing(&mut self) {
        while self.buffer.ends_with('/') {
            self.buffer.pop();
        }

        if self.buffer.chars().filter(|x| *x == '/').count() == 0 {
            self.buffer.push('/');
        }
    }

    pub fn parent(&mut self) {
        // let stems: Vec<&str> = self.buffer.split("/").filter(|a| !a.is_empty()).collect();

        let path_p = self.path_part();

        let mut iter = path_p.split('/');
        
        // Remove the last element from iterator.
        iter.next_back();

        let path = iter.collect::<Vec<_>>();

        let total = self.disk_name().to_owned() + ":/" + &path.join("/");
        
        self.buffer = total;
    }

    pub fn as_str(&self) -> &str {
        &self.buffer
    }

    pub fn into_string(self) -> String {
        self.buffer
    }
}

impl Display for Path {
    fn fmt(&self, f: &mut core::fmt::Formatter<'_>) -> core::fmt::Result {
        f.write_str(&self.buffer)
    }
}

impl PartialEq for Path {
    fn eq(&self, other: &Self) -> bool {
        self.buffer == other.buffer
    }
}

impl PartialEq<&str> for Path {
    fn eq(&self, other: &&str) -> bool {
        self.buffer == *other
    }
}