#![no_std]

extern crate alloc;

pub mod c_api;

use core::ffi::c_void;

use alloc::boxed::Box;
use alloc::string::String;
use alloc::vec;
use alloc::vec::Vec;

pub type OnDeviceOpenFn = extern "C" fn(*mut c_void);
pub type OnDeviceSetVolumeFn = extern "C" fn(*mut c_void, u8, u8) -> ();
pub type OnDeviceSetRateFn = extern "C" fn(*mut c_void, u32) -> ();
pub type OnDeviceWriteFn = extern "C" fn(*mut c_void, *const u8, usize) -> ();
pub type OnDeviceCloseFn = extern "C" fn(*mut c_void);

pub static mut AUDIO_DEVICE_GLOBAL_ID: usize = 0;
pub static mut AUDIO_DEVICES: Option<Vec<Box<dyn AudioDevice>>> = None;

pub struct GenericAudioDevice {
    name: String,
    private_data: *mut c_void,
    on_open: OnDeviceOpenFn,
    on_set_volume: OnDeviceSetVolumeFn,
    on_set_rate: OnDeviceSetRateFn,
    on_write: OnDeviceWriteFn,
    on_close: OnDeviceCloseFn,
}

pub trait AudioDevice {
    fn open(&self);
    fn set_volume(&self, l: u8, r: u8);
    fn set_rate(&self, rate: u32);
    fn write(&self, data: &[u8]);
    fn name(&self) -> &str;
    fn close(&self);
}

impl AudioDevice for GenericAudioDevice {
    fn open(&self) {
        (self.on_open)(self.private_data)
    }

    fn set_volume(&self, l: u8, r: u8) {
        (self.on_set_volume)(self.private_data, l, r);
    }

    fn set_rate(&self, rate: u32) {
        (self.on_set_rate)(self.private_data, rate);
    }

    fn write(&self, data: &[u8]) {
        (self.on_write)(self.private_data, data.as_ptr(), data.len());
    }

    fn name(&self) -> &str {
        &self.name
    }

    fn close(&self) {
        (self.on_close)(self.private_data);
    }
}

pub fn generate_id() -> usize {
    unsafe {
        let a = AUDIO_DEVICE_GLOBAL_ID;
        AUDIO_DEVICE_GLOBAL_ID += 1;
        a
    }
}

#[inline]
#[allow(static_mut_refs)]
pub fn get_device(index: usize) -> Option<&'static dyn AudioDevice> {
    let devs = unsafe { AUDIO_DEVICES.as_mut().expect("Not initialized!") };

    devs.get(index).map(|v| &**v)
}

pub fn init() {
    unsafe {
        AUDIO_DEVICES = Some(vec![]);
    }
}
