use core::{borrow::BorrowMut, ffi::{c_char, CStr}};

use alloc::{borrow::ToOwned, boxed::Box, ffi::CString, vec::Vec};
use noct_logger::qemu_note;

use crate::{generate_new_id, generic_drive::{ControlFn, GenericDrive, ReadFn, WriteFn}, structures::Drive, DRIVES};

#[unsafe(no_mangle)]
pub unsafe extern "C" fn diskman_register_drive(
    driver_name: *const c_char,
    id: *mut c_char,

    private_data: *mut u8,

    read: ReadFn,
    write: WriteFn,
    control: ControlFn,
) {
    assert!(!driver_name.is_null());
    assert!(!id.is_null());

    let name = unsafe { CStr::from_ptr(driver_name) };
    let id = unsafe { CString::from_raw(id) };

    qemu_note!("Name: {name:?}; ID: {id:?}");

    let drive = GenericDrive {
        driver_name: name.to_str().unwrap().to_owned(),
        id: id.into_string().unwrap(),
        private_data,
        read,
        write,
        control,
    };

    let mut a = DRIVES.lock();

    a.borrow_mut().get_mut().push(Box::new(drive));
}

#[unsafe(no_mangle)]
pub extern "C" fn diskman_generate_new_id(driver_name: *const c_char) -> *mut c_char {
    assert!(!driver_name.is_null());

    let cstr = unsafe { CStr::from_ptr(driver_name) };

    let id = generate_new_id(cstr.to_str().unwrap());

    CString::new(id).unwrap().into_raw()
}