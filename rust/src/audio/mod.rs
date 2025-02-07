use alloc::string::String;
use alloc::vec::Vec;

pub static mut AUDIO_DEVICES: Option<Vec<AudioDevice>> = None;

pub struct AudioDevice {
    name: String,
}
