use core::{
    cell::OnceCell,
    ffi::{c_char, CStr},
};
use noct_logger::qemu_note;

use crate::PSF;

#[no_mangle]
pub static mut PSF_FONT: OnceCell<PSF> = OnceCell::new();

#[no_mangle]
#[allow(static_mut_refs)]
pub unsafe extern "C" fn psf_init(file_path: *const c_char) {
    let path = CStr::from_ptr(file_path);
    let path = path.to_string_lossy().into_owned();

    unsafe {
        let psf = PSF::from_file(&path).expect("Failed to initialize PSF font.");

        PSF_FONT.set(psf).unwrap()
    };

    qemu_note!("Font initialized!");
}

#[no_mangle]
#[inline(never)]
pub fn rs_draw_character(psf: &OnceCell<PSF>, ch: u16, pos_x: usize, pos_y: usize, color: u32) {
    psf.get().unwrap().draw_character(ch, pos_x, pos_y, color);
}
