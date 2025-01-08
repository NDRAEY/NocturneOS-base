use core::arch::asm;
use core::cell::OnceCell;

use alloc::sync::Arc;
use alloc::vec;
use alloc::vec::Vec;
use spin::Mutex;

use crate::{qemu_err, qemu_ok};

static mut KEYBOARD_BUFFER: OnceCell<Arc<Mutex<KeyboardBuffer>>> = OnceCell::new();

pub struct KeyboardBuffer {
    buffer: Vec<u32>
}

impl KeyboardBuffer {
    pub fn new() -> Self {
        let buffer = vec![];

        KeyboardBuffer {
            buffer
        }
    }

    pub fn push(&mut self, character: u32) {
        self.buffer.push(character);
    }

    pub fn get(&mut self) -> u32 {
        while self.buffer.len() == 0 {
            unsafe { asm!("nop") };
        }

        self.buffer.pop().unwrap()
    }
}

#[no_mangle]
pub unsafe extern fn keyboard_buffer_init() {
    match KEYBOARD_BUFFER.set(Arc::new(Mutex::new(KeyboardBuffer::new()))) {
        Ok(()) => {
            qemu_ok!("Keyboard buffer initialized!");
        }
        Err(e) => {
            qemu_err!("Error initializing keyboard buffer");
        }
    };
}

#[no_mangle]
pub unsafe extern fn keyboard_buffer_put(character: u32) {
    let v = &KEYBOARD_BUFFER.get_mut().unwrap();
    let lk = Arc::clone(v);

    let mut val = lk.lock();
    val.push(character);
}