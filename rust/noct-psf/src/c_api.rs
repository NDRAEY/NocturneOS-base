use crate::PSF;

#[no_mangle]
pub fn draw_character(psf: &PSF, ch: u16, pos_x: usize, pos_y: usize, color: u32) {
    psf.draw_character(ch, pos_x, pos_y, color);
}