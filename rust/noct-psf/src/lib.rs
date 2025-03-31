#![no_std]

use noct_screen::set_pixel;

extern "C" {
    static psf_h: u8;
    fn psf1_get_glyph(ch: u16) -> *const u8;
}

pub extern "C" fn psf1_rupatch(c: u16) -> u16 {
    let hi: u8 = (c >> 8) as u8;
    let lo: u8 = (c & 0xff) as u8;

    let is_utf_compatible: bool = hi == 0xd0 || hi == 0xd1;

    if !is_utf_compatible {
        return c;
    }

    let mut t: u16 = 0;
    let sym: u16 = (lo & 0x3f) as u16;

    if sym == 0 {
        t = 224;
    } else if sym == 1 {
        t = if hi == 0xd0 { 240 } else { 225 };
    } else if (2..=15).contains(&sym) {
        t = 224 + sym;
    } else if sym == 16 {
        t = 128;
    } else if (18..=63).contains(&sym) {
        t = 128 + (sym - 16);
    } else if sym == 17 {
        t = if hi == 0xd1 { 241 } else { 129 };
    }

    t
}

#[no_mangle]
pub extern "C" fn draw_vga_ch(c: u16, pos_x: usize, pos_y: usize, color: u32) {
    let mask: [u8; 8] = [128, 64, 32, 16, 8, 4, 2, 1];

    let glyph_idx: u16 = psf1_rupatch(c);
    let raw_glyph: *const u8 = unsafe { psf1_get_glyph(glyph_idx) };

    if raw_glyph.is_null() {
        return;
    }

    let glyph = unsafe { core::slice::from_raw_parts(raw_glyph, psf_h as usize) };

    for (y, row) in glyph.iter().enumerate().take(unsafe { psf_h } as usize) {
        let rposy = pos_y + y;
        for (x, mask) in mask.iter().enumerate() {
            if (row & mask) != 0 {
                set_pixel(pos_x + x, rposy, color);
            }
        }
    }
}
