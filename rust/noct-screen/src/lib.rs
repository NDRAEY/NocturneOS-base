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
    let pixels = &mut back_framebuffer_mut()[offset..];

    pixels[0] = (color & 0xff) as u8;
    pixels[1] = ((color >> 8) & 0xff) as u8;
    pixels[2] = ((color >> 16) & 0xff) as u8;
}

#[inline]
pub fn back_framebuffer() -> &'static [u8] {
    let (w, h) = dimensions();
    let len = w * h * (bits_per_pixel() >> 3);

    unsafe { core::slice::from_raw_parts(back_framebuffer_addr, len) }
}

#[inline]
pub fn back_framebuffer_mut() -> &'static mut [u8] {
    // let (w, h) = dimensions();
    // let len = w * h * (bits_per_pixel() >> 3);

    unsafe { core::slice::from_raw_parts_mut(back_framebuffer_addr, framebuffer_size as _) }
}

#[inline]
pub fn framebuffer() -> &'static [u8] {
    let (w, h) = dimensions();
    let len = w * h * (bits_per_pixel() >> 3);

    unsafe { core::slice::from_raw_parts(framebuffer_addr, len) }
}

#[inline]
pub fn framebuffer_mut() -> &'static mut [u8] {
    let (w, h) = dimensions();
    let len = w * h * (bits_per_pixel() >> 3);

    unsafe { core::slice::from_raw_parts_mut(framebuffer_addr, len) }
}

#[inline]
pub fn dimensions() -> (usize, usize) {
    unsafe { (getScreenWidth() as usize, getScreenHeight() as usize) }
}

pub fn pitch() -> usize {
    unsafe { getDisplayPitch() as _ }
}

#[inline]
pub fn bits_per_pixel() -> usize {
    unsafe { framebuffer_bpp as _ }
}

#[inline]
pub fn flush() {
    let backfb = back_framebuffer();
    let real_screen = framebuffer_mut();

    real_screen.copy_from_slice(backfb);
}