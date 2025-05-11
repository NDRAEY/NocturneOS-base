use core::ffi::{c_char, c_void, CStr};

use alloc::{boxed::Box, string::ToString};
use noct_logger::qemu_note;

use crate::*;

use super::init;

#[unsafe(no_mangle)]
pub extern "C" fn audio_system_add_output(
    name: *mut c_char,
    priv_data: *mut c_void,
    open: OnDeviceOpenFn,
    set_volume: OnDeviceSetVolumeFn,
    set_rate: OnDeviceSetRateFn,
    write: OnDeviceWriteFn,
    close: OnDeviceCloseFn,
) {
    let name = unsafe { CStr::from_ptr(unsafe { name }) };
    let name = name.to_string_lossy();

    #[allow(static_mut_refs)]
    unsafe {
        AUDIO_DEVICES
            .as_mut()
            .expect("Not initialized!")
            .push(Box::new(GenericAudioDevice {
                name: name.to_string(),
                private_data: priv_data,
                on_open: core::mem::transmute(open),
                on_set_volume: core::mem::transmute(set_volume),
                on_set_rate: core::mem::transmute(set_rate),
                on_write: core::mem::transmute(write),
                on_close: core::mem::transmute(close),
            }))
    };

    qemu_note!("Added!");
}

#[unsafe(no_mangle)]
pub extern "C" fn audio_system_open(index: usize) -> bool {
    get_device(index).map(|a| a.open()).is_none()
}

#[unsafe(no_mangle)]
pub extern "C" fn audio_system_set_volume(index: usize, left: u8, right: u8) {
    get_device(index)
        .expect("No device!")
        .set_volume(left, right);
}

#[unsafe(no_mangle)]
pub extern "C" fn audio_system_rate(index: usize, rate: u32) {
    get_device(index).expect("No device!").set_rate(rate);
}

#[unsafe(no_mangle)]
pub extern "C" fn audio_system_write(index: usize, data: *const u8, len: usize) {
    get_device(index)
        .expect("No device!")
        .write(unsafe { core::slice::from_raw_parts(unsafe { data }, len) });
}

#[unsafe(no_mangle)]
pub extern "C" fn audio_system_close(index: usize) {
    get_device(index).expect("No device!").close();
}

#[unsafe(no_mangle)]
#[allow(static_mut_refs)]
pub extern "C" fn audio_system_get_default_device_index() -> usize {
    let devs = unsafe { AUDIO_DEVICES.as_mut().expect("Not initialized!") };

    if devs.is_empty() {
        return 0xFFFF_FFFF;
    }

    devs.len() - 1
}

#[unsafe(no_mangle)]
pub extern "C" fn audio_system_init() {
    init();
}
