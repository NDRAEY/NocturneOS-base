use alloc::vec;
use alloc::vec::Vec;

pub static mut AUDIO_DEVICE_GLOBAL_ID: usize = 0;
pub static mut AUDIO_DEVICES: Option<Vec<&dyn AudioDevice>> = None;

pub trait AudioDevice {
    fn open(&self);
    fn set_volume(&self, l: u8, r: u8);
    fn set_rate(&self, rate: u32);
    fn write(&self, data: &[u8]);
    fn close(&self);
}

pub fn generate_id() -> usize {
    unsafe {
        let a = AUDIO_DEVICE_GLOBAL_ID;
        AUDIO_DEVICE_GLOBAL_ID += 1;
        a
    }
}

pub fn init() {
    unsafe {
        AUDIO_DEVICES = Some(vec![]);
    }
}

#[no_mangle]
pub fn audio_system_init() {
    init();
}
