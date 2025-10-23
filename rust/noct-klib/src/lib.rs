#![no_std]

use core::ffi::c_char;

#[unsafe(no_mangle)]
pub unsafe extern "C" fn strlen(input: *const c_char) -> usize {
    let mut len = 0;

    while input.add(len).read() != 0 {
        len += 1;
    }

    len
}
