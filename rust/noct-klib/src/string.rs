use core::ffi::c_char;

#[unsafe(no_mangle)]
pub unsafe extern "C" fn strcpy(dest: *mut c_char, src: *const c_char) -> *const c_char {
    let mut i: usize = 0;

    unsafe {
        while src.add(i).read() != 0 {
            dest.add(i).write(src.add(i).read());
            i += 1;
        }

        dest.add(i).write(0);
    }

    dest.cast()
}

#[unsafe(no_mangle)]
pub extern "C" fn isalnum(c: c_char) -> bool {
    (c >= '0' as _ && c <= '9' as _)
        || (c >= 'a' as _ && c <= 'z' as _)
        || (c >= 'A' as _ && c <= 'Z' as _)
}

#[unsafe(no_mangle)]
pub unsafe extern "C" fn strncpy(dest: *mut c_char, src: *const c_char, n: usize) -> *mut c_char {
    for i in 0..n {
        if unsafe { src.add(i).read() } == 0 {
            break;
        }

		unsafe { dest.add(i).write(src.add(i).read()) };
	}

	return dest;
}