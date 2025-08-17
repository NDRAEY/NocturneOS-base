use core::ffi::{CStr, c_char, c_longlong, c_uint, c_ulonglong};

use alloc::{borrow::ToOwned, boxed::Box, ffi::CString};
use noct_logger::qemu_note;
use num_traits::FromPrimitive;

use crate::{
    generate_new_id,
    generic_drive::{ControlFn, GenericDrive, ReadFn, WriteFn},
    structures::Command,
};

/// Register a drive from external driver handle
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

    crate::register_drive(Box::new(drive));
}

/// Generates a new id for disk, according to driver identificator
/// 
/// # Safety: `driver_name` must not be null.
#[unsafe(no_mangle)]
pub unsafe extern "C" fn diskman_generate_new_id(driver_name: *const c_char) -> *mut c_char {
    assert!(!driver_name.is_null());

    let cstr = unsafe { CStr::from_ptr(driver_name) };

    let id = generate_new_id(cstr.to_str().unwrap());

    CString::new(id).unwrap().into_raw()
}

/// Read data from disk.
/// 
/// Returns -1 when `disk_id == NULL` or `buffer == NULL`
/// 
/// # Arguments:
/// 
/// * `location` - Exact location in bytes.
/// * `size` - Exact size in bytes.
/// * `buffer` - Pointer to output buffer where data is being stored.
#[unsafe(no_mangle)]
pub unsafe extern "C" fn diskman_read(
    disk_id: *const c_char,
    location: c_ulonglong,
    size: c_ulonglong,
    buffer: *mut u8,
) -> c_longlong {
    if disk_id.is_null() || buffer.is_null() {
        return -1;
    }

    let disk_id = unsafe { CStr::from_ptr(disk_id) };
    let buffer = unsafe { core::slice::from_raw_parts_mut(buffer, size as _) };

    crate::read(&disk_id.to_string_lossy(), location, buffer)
}

/// Write data to disk.
/// 
/// Returns -1 when `disk_id == NULL` or `buffer == NULL`
/// 
/// # Arguments:
/// 
/// * `location` - Exact location in bytes.
/// * `size` - Exact size in bytes.
/// * `buffer` - Pointer to input buffer where data is being copied to disk.
#[unsafe(no_mangle)]
pub unsafe extern "C" fn diskman_write(
    disk_id: *const c_char,
    location: c_ulonglong,
    size: c_ulonglong,
    buffer: *const u8,
) -> c_longlong {
    if disk_id.is_null() || buffer.is_null() {
        return -1;
    }

    let disk_id = unsafe { CStr::from_ptr(disk_id) };
    let buffer = unsafe { core::slice::from_raw_parts(buffer, size as _) };

    crate::write(&disk_id.to_string_lossy(), location, buffer)
}

/// Controls disk/drive.
/// 
/// If `command_parameters` is `NULL`, function assumes that there's no input parameters.
/// If `output_data` is `NULL`, function assumes that no data will be written.
/// 
/// Returns -1 when `disk_id == NULL`
/// 
/// # Arguments:
/// 
/// * `command` - A command. Look [crate::structures::Command] for more.
/// * `command_parameters` - Some parameters for the command (may be NULL).
/// * `command_parameters_length` - Length of command parameters buffer.
/// * `output_data` - Pointer to output data buffer (may be NULL).
/// * `output_data_length` - Length of output data buffer.
#[unsafe(no_mangle)]
pub unsafe extern "C" fn diskman_control(
    disk_id: *const c_char,
    command: c_uint,
    command_parameters: *const u8,
    command_parameters_length: c_uint,
    output_data: *mut u8,
    output_data_length: c_uint,
) -> c_longlong {
    if disk_id.is_null() {
        return -1;
    }

    let disk_id = unsafe { CStr::from_ptr(disk_id) };

    let command_parameters = if command_parameters.is_null() {
        &[]
    } else {
        unsafe { core::slice::from_raw_parts(command_parameters, command_parameters_length as _) }
    };

    let data = if output_data.is_null() {
        &mut []
    } else {
        unsafe { core::slice::from_raw_parts_mut(output_data, output_data_length as _) }
    };

    crate::control(
        &disk_id.to_string_lossy(),
        Command::from_u32(command).unwrap(),
        command_parameters,
        data,
    )
}
