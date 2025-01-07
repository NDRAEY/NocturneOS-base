pub mod font;
pub mod point;

extern "C" {
    static back_framebuffer_addr: *mut u8;
    static framebuffer_bpp: u32;
    static framebuffer_pitch: u32;
}

#[inline(always)]
pub unsafe fn set_pixel(x: u32, y: u32, color: u32) {
    let offset: usize = ((x * (framebuffer_bpp >> 3)) + y * framebuffer_pitch) as usize;
    let pixels: *mut u8 = back_framebuffer_addr.wrapping_add(offset);

    pixels.wrapping_add(0).write((color & 0xff) as u8);
    pixels.wrapping_add(1).write(((color >> 8) & 255) as u8);
    pixels.wrapping_add(2).write(((color >> 16) & 255) as u8);
}
