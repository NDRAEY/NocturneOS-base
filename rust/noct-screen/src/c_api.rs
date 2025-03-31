use crate::{
    manager::{Manager, GLOBAL_SCREENS},
    screen::{PixelFormat, Screen},
};

use num_traits::FromPrimitive;

#[no_mangle]
pub extern "C" fn screenman_init() {
    let result = unsafe { GLOBAL_SCREENS.write().set(Manager::new()) };

    if result.is_err() {
        panic!("Failed to initialize screen manager");
    }
}

#[no_mangle]
pub extern "C" fn screenman_add_screen(
    framebuffer: *mut u8,
    width: u32,
    height: u32,
    pixel_format: u32,
) -> u32 {
    let pixfmt = PixelFormat::from_u32(pixel_format).expect("Failed to get PixelFormat");

    let screen =
        Screen::from_raw_pointer(framebuffer, width as usize, height as usize, pixfmt);

    let mgr = unsafe { &GLOBAL_SCREENS };
    let mut mgr = mgr.write();

    let mgr = mgr.get_mut().expect("Screen manager is not initialized.");

    mgr.add_screen(screen) as u32
}

#[no_mangle]
pub extern "C" fn screenman_get_default_screen_index() -> u32 {
    let mgr = unsafe { &GLOBAL_SCREENS }.write();

    mgr.get()
        .unwrap()
        .default_screen_index()
        .unwrap_or(0xffffffff) as _
}

#[no_mangle]
pub extern "C" fn screenman_get_screen_parameters(
    index: u32,
    out_width: *mut u32,
    out_height: *mut u32,
    out_size: *mut usize,
    out_bpp: *mut u32,
    out_framebuffer: *mut *const u8
) {
    let mut mgr = unsafe { &GLOBAL_SCREENS }.read();
    let mgr = mgr.get().expect("Screen manager is not initialized.");
    let screen = mgr.nth_screen(index as _);

    if screen.is_none() {
        unsafe { *out_size = 0 };

        return;
    }

    let screen = screen.unwrap();

    unsafe { *out_width = screen.resolution().width as u32; }
    unsafe { *out_height = screen.resolution().height as u32; }
    unsafe { *out_bpp = screen.pixel_format().bits_per_pixel() as u32; }
    unsafe {
        *out_framebuffer = screen.framebuffer_raw().as_ptr();
    }
}
