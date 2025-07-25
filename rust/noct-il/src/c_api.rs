use core::ffi::{c_char, CStr};

use crate::{info, INTERNAL_LOGGER};

#[unsafe(no_mangle)]
pub unsafe extern "C" fn il_log(message: *const c_char) {
    let mystr = unsafe { CStr::from_ptr(message) };
    let str = mystr.to_string_lossy().into_owned();

    unsafe { INTERNAL_LOGGER.force_write_unlock() };

    info(str);
}