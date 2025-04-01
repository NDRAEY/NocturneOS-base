#![no_std]
#![allow(non_upper_case_globals)]
#![allow(non_camel_case_types)]
#![allow(non_snake_case)]

include!(concat!(env!("OUT_DIR"), "/bindings.rs"));

pub fn fill(color: u32) {
    let (w, h) = dimensions();

    for y in 0..h {
        for x in 0..w {
            set_pixel(x, y, color);
        }
    }
}

pub fn set_pixel(x: usize, y: usize, color: u32) {
    let (w, h) = dimensions();

    if x >= w || y >= h {
        return;
    }

    let offset = (x * (bits_per_pixel() >> 3)) + y * pitch();
    let pixels = &mut framebuffer_mut()[offset..];

    pixels[0] = (color & 0xff) as u8;
    pixels[1] = ((color >> 8) & 0xff) as u8;
    pixels[2] = ((color >> 16) & 0xff) as u8;
}

#[inline]
pub fn framebuffer() -> &'static [u8] {
    let (w, h) = dimensions();
    let len = w * h * bits_per_pixel();

    unsafe { core::slice::from_raw_parts(back_framebuffer_addr, len) }
}

#[inline]
pub fn framebuffer_mut() -> &'static mut [u8] {
    let (w, h) = dimensions();
    let len = w * h * bits_per_pixel();

    unsafe { core::slice::from_raw_parts_mut(back_framebuffer_addr, len) }
}

#[inline]
pub fn dimensions() -> (usize, usize) {
    unsafe { (getScreenWidth().try_into().unwrap(), getScreenHeight().try_into().unwrap()) }
}

pub fn pitch() -> usize {
    unsafe { getDisplayPitch() as _ }
}

#[inline]
pub fn bits_per_pixel() -> usize {
    unsafe { framebuffer_bpp as _ }
}