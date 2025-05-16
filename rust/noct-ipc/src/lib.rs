#![no_std]

use alloc::{
    string::{String, ToString},
    vec::Vec,
};

extern crate alloc;

pub mod manager;

#[derive(Debug)]
pub struct NamedChannel {
    name: String,

    // creator: usize, // Process PID
    // connected_processes: Vec<usize>,
    data: Vec<u8>,
}

impl NamedChannel {
    pub fn new<S: ToString>(name: S) -> Option<Self> {
        Some(Self {
            name: name.to_string(),
            // creator: noct_sched::me().pid as usize,
            // connected_processes: Vec::new(),
            data: Vec::with_capacity(256),
        })
    }

    // fn handle_connection(&mut self, from_pid: usize) {
    //     self.connected_processes.push(from_pid);
    // }

    pub fn write(&mut self, data: &[u8]) {
        self.data.extend_from_slice(data);
    }

    pub fn read(&mut self, data: &mut [u8]) -> usize {
        let read_size = core::cmp::min(data.len(), self.data.len());

        data.copy_from_slice(&self.data[..read_size]);

        self.data = self.data[data.len()..].to_vec();

        read_size
    }
}
