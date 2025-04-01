use crate::{
    screen::{PixelFormat, Screen},
    GLOBAL_SCREEN,
};

use num_traits::FromPrimitive;

#[no_mangle]
pub extern "C" fn screen_init(framebuffer: *mut u8, width: u32, height: u32, pixel_format: u32) {
    let pixfmt = PixelFormat::from_u32(pixel_format).expect("Failed to get PixelFormat");

    let screen = Screen::from_raw_pointer(framebuffer, width as usize, height as usize, pixfmt);

    let mgr = unsafe { &mut GLOBAL_SCREEN };
    mgr.set(screen).expect("Failed to set the screen.");
}

#[no_mangle]
pub extern "C" fn screen_get(
    out_width: *mut u32,
    out_height: *mut u32,
    out_size: *mut usize,
    out_bpp: *mut u32,
    out_framebuffer: *mut *const u8,
) {
    let screen = unsafe { &GLOBAL_SCREEN }
        .get()
        .expect("Screen is not initialized.");

    unsafe {
        *out_width = screen.resolution().width as u32;
        *out_height = screen.resolution().height as u32;
        *out_size = (screen.stride() * screen.resolution().height) as usize;
        *out_bpp = screen.pixel_format().bits_per_pixel() as u32;
        *out_framebuffer = screen.framebuffer_raw().as_ptr();
    }
}

#[no_mangle]
pub extern "C" fn screen_set(width: u32, height: u32, bpp: u32, framebuffer: *mut u8) {
    let screen = unsafe { &mut GLOBAL_SCREEN }
        .get_mut()
        .expect("Screen is not initialized.");

    let len = width * height * (bpp >> 3);

    unsafe {
        screen.pixel_format = {
            if bpp == 24 {
                PixelFormat::RGB
            } else {
                todo!("More pixel formats!")
            }
        };

        screen.resolution.width = width as _;
        screen.resolution.height = height as _;
        screen.framebuffer = core::slice::from_raw_parts_mut(framebuffer, len as _);
    }
}
