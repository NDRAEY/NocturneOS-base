#![no_std]

extern crate alloc;

pub const PS2_DATA_PORT: u16 = 0x60;
pub const PS2_STATE_REG: u16 = 0x64;

pub mod controller;
pub mod keyboard;
