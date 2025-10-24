use core::ffi::{CStr, c_char};

use crate::{INTERNAL_LOGGER, info};

#[unsafe(no_mangle)]
pub unsafe extern "C" fn il_log(message: *const c_char) {
    let mystr = unsafe { CStr::from_ptr(message) };
    let str = mystr.to_string_lossy().into_owned();

    unsafe { INTERNAL_LOGGER.force_write_unlock() };

    info(str);
}
