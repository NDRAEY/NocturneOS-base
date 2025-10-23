use core::ffi::c_char;

#[unsafe(no_mangle)]
pub unsafe extern "C" fn strlen(input: *const c_char) -> usize {
    debug_assert!(!input.is_null());

    let mut len = 0;

    while unsafe { input.add(len).read() } != 0 {
        len += 1;
    }

    len
}

#[unsafe(no_mangle)]
pub unsafe extern "C" fn strcmp(mut stra: *const c_char, mut strb: *const c_char) -> isize {
    debug_assert!(!stra.is_null());
    debug_assert!(!strb.is_null());

    unsafe {
        while stra.read() != 0 && stra.read() == strb.read() {
            stra = stra.add(1);
            strb = strb.add(1);
        }

        (stra.read() - strb.read()) as _
    }
}

#[unsafe(no_mangle)]
pub unsafe extern "C" fn strcpy(dest: *mut c_char, src: *const c_char) -> isize {
    let mut i: usize = 0;

    unsafe {
        while src.add(i).read() != 0 {
            dest.add(i).write(src.add(i).read());
            i += 1;
        }

        dest.add(i).write(0);
    }

    i as _
}

#[unsafe(no_mangle)]
pub extern "C" fn isalnum(c: c_char) -> bool {
    (c >= '0' as _ && c <= '9' as _)
        || (c >= 'a' as _ && c <= 'z' as _)
        || (c >= 'A' as _ && c <= 'Z' as _)
}
