use core::cell::OnceCell;

use noct_psf::PSF;

use crate::console::{AttributeValue, Console};

unsafe extern "C" {
    #[allow(improper_ctypes)]
    pub static PSF_FONT: OnceCell<PSF>;
}

pub fn render(console: &mut Console) {
    let dimensions = console.dimensions();
    let mut current_color = 0x00_ffffff;

    for row in 0..dimensions.rows {
        for column in 0..dimensions.columns {
            let char = console.get_character(row, column).unwrap();

            if *char == char::default() {
                continue;
            }

            if let Some(attribute) = console.get_attribute(row, column) {
                match attribute.attribute_value {
                    AttributeValue::Color(color) => {
                        current_color = color;
                    }
                    _ => (),
                }
            }

            unsafe {
                PSF_FONT.get().unwrap().draw_character(
                    *char as u16,
                    column * 8,
                    row * 16,
                    current_color,
                )
            };
        }
    }
}
