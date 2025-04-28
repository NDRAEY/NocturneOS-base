#![no_std]

unsafe extern "C" {
    pub fn getTicks() -> usize;
    pub fn getFrequency() -> usize;
    pub fn sleep_ms(milliseconds: u32);
}

#[inline]
pub fn sleep(_d: u32) {
    unsafe { sleep_ms(_d * 1000) };
}

#[inline]
pub fn timestamp() -> usize {
    unsafe { getTicks() / (getFrequency() / 1000) }
}