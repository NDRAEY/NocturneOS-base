use core::ffi::{c_char, CStr};

unsafe extern "C" {
    static VERSION_MAJOR: usize;
    static VERSION_MINOR: usize;
    static VERSION_PATCH: usize;

    static VERSION_NAME: *const c_char;
    static OS_ARCH: *const c_char;
}

pub fn architecture() -> &'static str {
    let instring = unsafe { CStr::from_ptr(OS_ARCH) };

    instring.to_str().unwrap()
}

pub fn version_name() -> &'static str {
    let instring = unsafe { CStr::from_ptr(VERSION_NAME) };

    instring.to_str().unwrap()
}

pub fn version() -> (usize, usize, usize) {
    unsafe { (VERSION_MAJOR, VERSION_MINOR, VERSION_PATCH) }
}
