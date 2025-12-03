#![no_std]
#![allow(static_mut_refs)]

extern crate alloc;

use alloc::vec::Vec;
use core::cell::OnceCell;

use noct_logger::{qemu_err, qemu_ok};
use noct_sched::task_yield;

use spin::RwLock;

pub mod kbd;

/// Global keyboard buffer (kernel-wide). Used everywhere.
static mut KEYBOARD_BUFFER: OnceCell<KeyboardBuffer> = OnceCell::new();

/// A definition of Keyboard buffer. It's just a pre-allocated vector, but functionality can be extended soon and new fields may be added.
pub struct KeyboardBuffer {
    buffer: Vec<u32>,
}

impl Default for KeyboardBuffer {
    fn default() -> Self {
        Self::new()
    }
}

impl KeyboardBuffer {
    /// Creates a new keyboard buffer instance.
    pub fn new() -> Self {
        let buffer = Vec::<u32>::with_capacity(512);

        KeyboardBuffer { buffer }
    }

    /// Push a character into a buffer.
    pub fn push(&mut self, character: u32) {
        let bind = &mut self.buffer;

        bind.push(character);
    }

    /// Pop a character from a buffer, just calls for Vec::pop().
    pub fn get_raw(&mut self) -> Option<u32> {
        let bind = &mut self.buffer;
        
        bind.pop()
    }

    /// Polls for a character from a buffer.
    pub fn get(&mut self) -> u32 {
        while self.buffer.is_empty() {
            // Idk what this function does, but it works! (Not throwing polling away)
            core::sync::atomic::compiler_fence(core::sync::atomic::Ordering::SeqCst);
            task_yield();
        }

        self.get_raw().unwrap()
    }
}

///
///  External function for C API that initializes the global keyboard buffer.
///
/// # Safety
/// - Keyboard buffer MUST be called before others
#[unsafe(no_mangle)]
pub unsafe extern "C" fn keyboard_buffer_init() {
    match unsafe { KEYBOARD_BUFFER.set(KeyboardBuffer::new()) } {
        Ok(()) => {
            qemu_ok!("Keyboard buffer initialized!");
        }
        Err(_) => {
            qemu_err!("Error initializing keyboard buffer");
        }
    };
}

/// External function for C API that puts a character into a buffer.
#[unsafe(no_mangle)]
pub extern "C" fn keyboard_buffer_put(character: u32) {
    unsafe { KEYBOARD_BUFFER.get_mut().unwrap().push(character) };
}

/// External function for C API that gets a character from a buffer.
#[unsafe(no_mangle)]
pub extern "C" fn keyboard_buffer_get() -> u32 {
    let v = unsafe { KEYBOARD_BUFFER.get_mut().unwrap() };

    v.get()
}

/// External function for C API that gets a character from a buffer.
/// This function returns 0 if no characters in the buffer to make it compatible with C API.
#[unsafe(no_mangle)]
pub extern "C" fn keyboard_buffer_get_or_nothing() -> u32 {
    let v = unsafe { KEYBOARD_BUFFER.get_mut().unwrap() };

    v.get_raw().unwrap_or(0)
}
