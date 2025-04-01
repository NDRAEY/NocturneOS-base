use core::cell::OnceCell;

use noct_psf::PSF;

use crate::console::Console;

unsafe extern {
    pub static PSF_FONT: OnceCell<PSF>;
}

pub fn render(console: &Console) {
    let dimensions = console.dimensions();

    for row in 0..dimensions.rows {
        for column in 0..dimensions.columns {
            let char = console.get_character(row, column).unwrap();

            if char.character == char::default() {
                continue;
            }

            unsafe { PSF_FONT.get().unwrap().draw_character(char.character as u16, column * 8, row * 16, char.attribute.color()) };
        }
    }
}