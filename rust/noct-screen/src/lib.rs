#![no_std]
#![allow(non_upper_case_globals)]
#![allow(non_camel_case_types)]
#![allow(non_snake_case)]

include!(concat!(env!("OUT_DIR"), "/bindings.rs"));

pub fn set_pixel(x: u32, y: u32, color: u32) {
    let pixels = unsafe {
        back_framebuffer_addr.add(((x * (framebuffer_bpp >> 3)) + y * framebuffer_pitch) as usize)
    };
    let pixels = unsafe {
        core::slice::from_raw_parts_mut(pixels, (getDisplayPitch() * getScreenHeight()) as _)
    };

    pixels[0] = (color & 0xff) as u8;
    pixels[1] = ((color >> 8) & 0xff) as u8;
    pixels[2] = ((color >> 16) & 0xff) as u8;
}
