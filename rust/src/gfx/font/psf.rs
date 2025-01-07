use crate::{gfx::set_pixel, qemu_log};

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
    } else if sym >= 2 && sym <= 15 {
        t = 224 + sym;
    } else if sym == 16 {
        t = 128;
    } else if sym >= 18 && sym <= 63 {
        t = 128 + (sym - 16);
    } else if sym == 17 {
        t = if hi == 0xd1 { 241 } else { 129 };
    }

    return t;
}

#[no_mangle]
pub extern "C" fn draw_vga_ch(c: u16, pos_x: usize, pos_y: usize, color: u32) {
    let mask: [u8; 8] = [128, 64, 32, 16, 8, 4, 2, 1];

    let glyph_idx: u16 = psf1_rupatch(c);
    let raw_glyph: *const u8 = unsafe { psf1_get_glyph(glyph_idx) };

    // qemu_log!("Height is: {}", unsafe{psf_h} as usize);

    if raw_glyph.is_null() {
        return;
    }

    let glyph = unsafe { core::slice::from_raw_parts(raw_glyph, psf_h as usize) };

    // for let y = 0; y < _h; y+=1 {
    for y in 0..unsafe{psf_h} as u32 {
        let rposy = pos_y as u32 + y;
        for x in 0..8 {
            // if (glyph[y] & (1 << (8 - x))) {
            if (glyph[y as usize] & mask[x]) != 0 {
                unsafe { set_pixel((pos_x + x) as u32, rposy, color) };
            }
        }
    }
}
