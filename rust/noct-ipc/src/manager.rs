#![allow(static_mut_refs)]

use core::cell::OnceCell;

use alloc::{
    string::{String, ToString},
    vec::Vec,
};
use spin::RwLock;

use crate::NamedChannel;

static mut NAMED_CHANNELS: RwLock<OnceCell<Manager>> = RwLock::new(OnceCell::new());

#[derive(Debug)]
pub struct Manager {
    channels: Vec<NamedChannel>,
}

impl Manager {
    pub fn new() -> Self {
        Self {
            channels: Vec::new(),
        }
    }

    pub fn create<S: ToString>(&mut self, name: S) -> Option<Cookie> {
        let chan = NamedChannel::new(name.to_string().clone());

        if chan.is_none() {
            return None;
        }

        self.channels.push(chan.unwrap());

        Some(Cookie { name: name.to_string() })
    }

    pub fn find<S: ToString>(&self, name: S) -> Option<Cookie> {
        for i in &self.channels {
            if i.name == name.to_string() {
                return Some(Cookie { name: name.to_string() })
            }
        }

        None
    }

    pub fn read<S: ToString>(&mut self, name: S, data: &mut [u8]) {
        for i in &mut self.channels {
            if i.name == name.to_string() {
                i.read(data);
            }
        }
    }

    pub fn write<S: ToString>(&mut self, name: S, data: &[u8]) {
        for i in &mut self.channels {
            if i.name == name.to_string() {
                i.write(data);
            }
        }
    }
}

pub struct Cookie {
    name: String,
}

impl Cookie {
    pub fn read(&self, data: &mut [u8]) {
        unsafe { NAMED_CHANNELS.write().get_mut().unwrap().read(&self.name, data) };
    }
    
    pub fn write(&self, data: &[u8]) {
        unsafe { NAMED_CHANNELS.write().get_mut().unwrap().write(&self.name, data) };
    }
}

#[unsafe(no_mangle)]
pub fn ipc_init() {
    unsafe { NAMED_CHANNELS.write().set(Manager::new()).unwrap() };
}

pub fn create_named_channel<S: ToString>(name: S) -> Option<Cookie> {
    unsafe { NAMED_CHANNELS.write().get_mut().unwrap().create(name) }
}

pub fn find_named_channel<S: ToString>(name: S) -> Option<Cookie> {
    unsafe { NAMED_CHANNELS.read().get().unwrap().find(name) }
}
