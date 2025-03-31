use noct_psf::draw_vga_ch;

use crate::console::Console;

pub fn render(console: &Console) {
    let dimensions = console.dimensions();

    for row in 0..dimensions.rows {
        for column in 0..dimensions.columns {
            let char = console.get_character(row, column).unwrap();

            if char.character == char::default() {
                continue;
            }

            draw_vga_ch(char.character as u16, column * 8, row * 16, char.attribute.color());
        }
    }
}