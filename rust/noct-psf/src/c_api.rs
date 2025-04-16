use core::cell::OnceCell;

use noct_logger::qemu_note;

use crate::PSF;

#[no_mangle]
#[inline(never)]
pub fn rs_draw_character(psf: &OnceCell<PSF>, ch: u16, pos_x: usize, pos_y: usize, color: u32) {
    psf.get().unwrap().draw_character(ch, pos_x, pos_y, color);
}