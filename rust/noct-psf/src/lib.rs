#![no_std]

extern crate alloc;

use alloc::vec::Vec;
use noct_logger::qemu_err;
use noct_screen::set_pixel;

pub mod c_api;

#[derive(Debug)]
#[repr(C)]
pub struct PSF {
    font_data: Vec<u8>,
    mode: usize,
    height: usize,
}

impl PSF {
    pub unsafe fn from_raw_ptr(data: *const u8, len: usize) -> Option<Self> {
        if data.is_null() {
            panic!("Attempted to pass null to PSF!");
        }

        let data = core::slice::from_raw_parts(data, len);

        if data[0] != 0x36 || data[1] != 0x04 {
            return None;
        }

        let mode = data[2] as usize;
        let height = data[3] as usize;

        Some(Self {
            font_data: Vec::from(data),
            mode,
            height,
        })
    }

    pub fn from_data(data: &[u8]) -> Option<Self> {
        let mode = data[2] as usize;
        let height = data[3] as usize;

        if data[0] != 0x36 || data[1] != 0x04 {
            return None;
        }

        Some(Self {
            font_data: Vec::from(data),
            mode,
            height,
        })
    }

    pub fn from_data_vec(data: Vec<u8>) -> Option<Self> {
        let mode = data[2] as usize;
        let height = data[3] as usize;

        if data[0] != 0x36 || data[1] != 0x04 {
            return None;
        }

        Some(Self {
            font_data: data,
            mode,
            height,
        })
    }

    pub fn from_file(path: &str) -> Option<Self> {
        let data = match noct_fs::read(path) {
            Ok(f) => f,
            Err(e) => {
                qemu_err!("Error: {e}");

                return None;
            }
        };

        Self::from_data_vec(data)
    }

    fn psf1_rupatch(c: u16) -> u16 {
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

    fn get_glyph(&self, ch: u16) -> Option<&[u8]> {
        if (ch > 511) || (ch > 255 && (self.mode == 0 || self.mode == 2)) {
            return None;
        }

        let offset = 4 + ch as usize * self.height;

        Some(&self.font_data[offset..offset + self.height])
    }

    pub fn draw_character_custom(
        &self,
        c: u16,
        pos_x: usize,
        pos_y: usize,
        color: u32,
        callback: fn(usize, usize, u32) -> (),
    ) {
        let mask: [u8; 8] = [128, 64, 32, 16, 8, 4, 2, 1];

        let glyph_idx: u16 = Self::psf1_rupatch(c);
        let raw_glyph = self.get_glyph(glyph_idx);

        if raw_glyph.is_none() {
            qemu_err!("Glyph is None");
            return;
        }

        let glyph = raw_glyph.unwrap();

        for (y, row) in glyph.iter().enumerate() {
            let rposy = pos_y + y;

            for (x, mask) in mask.iter().enumerate() {
                if (row & mask) != 0 {
                    callback(pos_x + x, rposy, color);
                }
            }
        }
    }

    #[inline]
    pub fn draw_character(&self, c: u16, pos_x: usize, pos_y: usize, color: u32) {
        self.draw_character_custom(c, pos_x, pos_y, color, |x, y, color| set_pixel(x, y, color));
    }
}
