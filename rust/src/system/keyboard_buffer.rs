use core::arch::asm;
use core::cell::OnceCell;

use alloc::vec;
use alloc::vec::Vec;

use crate::{qemu_err, qemu_ok};

static mut KEYBOARD_BUFFER: OnceCell<KeyboardBuffer> = OnceCell::new();

pub struct KeyboardBuffer {
    buffer: Vec<u32>,
}

impl KeyboardBuffer {
    pub fn new() -> Self {
        let buffer = vec![];

        KeyboardBuffer { buffer }
    }

    pub fn push(&mut self, character: u32) {
        self.buffer.push(character);
    }

    pub fn get(&mut self) -> u32 {
        while self.buffer.is_empty() {
            core::sync::atomic::compiler_fence(core::sync::atomic::Ordering::SeqCst);
            unsafe { asm!("nop", options(nomem, nostack, preserves_flags)) };
        }

        self.buffer.pop().unwrap()
    }
}

#[no_mangle]
pub unsafe extern "C" fn keyboard_buffer_init() {
    match KEYBOARD_BUFFER.set(KeyboardBuffer::new()) {
        Ok(()) => {
            qemu_ok!("Keyboard buffer initialized!");
        }
        Err(e) => {
            qemu_err!("Error initializing keyboard buffer");
        }
    };
}

#[no_mangle]
pub unsafe extern "C" fn keyboard_buffer_put(character: u32) {
    let v = KEYBOARD_BUFFER.get_mut().unwrap();

    v.push(character);
}

#[no_mangle]
pub unsafe extern "C" fn keyboard_buffer_get() -> u32 {
    let v = KEYBOARD_BUFFER.get_mut().unwrap();

    v.get()
}
