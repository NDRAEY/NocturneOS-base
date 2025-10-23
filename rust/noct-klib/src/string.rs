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

#[unsafe(no_mangle)]
pub unsafe extern "C" fn strcmp(mut stra: *const c_char, mut strb: *const c_char) -> isize {
	while(stra.read() != 0 && stra.read() == strb.read()) {
		stra = stra.add(1);
		strb = strb.add(1);
	}

	return (stra.read() - strb.read()) as _;
}
