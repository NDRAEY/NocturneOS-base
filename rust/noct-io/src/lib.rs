#![no_std]

unsafe extern "C" {
    pub fn getchar() -> u32;
}