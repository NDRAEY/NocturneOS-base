use alloc::vec;
use alloc::vec::Vec;

pub enum ForeignAttributes {
    InvertedColors = (1 << 0),
}

#[derive(Clone, Copy, Debug, Default)]
pub struct Attribute(u32);

impl Attribute {
    pub fn color(&self) -> u32 {
        self.0 & 0x00_ffffff
    }

    pub fn foreign(&self) -> u8 {
        ((self.0 & 0xff_000000) >> 24) as u8
    }

    pub fn set_color(&mut self, color: u32) {
        let color = color & 0x00_ffffff;

        self.0 = (self.0 & 0xff_000000) | color;
    }

    pub fn set_foreign(&mut self, foreign: u8) {
        let foreign = (foreign as u32) << 24;

        self.0 = self.color() | foreign;
    }
}

#[derive(Clone, Default)]
pub struct Character {
    pub character: char,
    pub attribute: Attribute,
}

pub struct Dimensions {
    pub rows: usize,
    pub columns: usize,
}

pub struct Console {
    dimensions: Dimensions,
    row: usize,
    column: usize,
    data: Vec<Character>,
}

impl Console {
    pub fn new(rows: usize, columns: usize) -> Self {
        Self {
            dimensions: Dimensions { rows, columns },
            row: 0,
            column: 0,
            data: vec![Character::default(); rows * columns],
        }
    }

    pub fn dimensions(&self) -> &Dimensions {
        &self.dimensions
    }

    pub fn data_position(&self) -> usize {
        (self.row * self.dimensions.columns) + self.column
    }

    pub fn get_character(&self, row: usize, column: usize) -> Option<&Character> {
        self.data.get((row * self.dimensions.columns) + column)
    }

    pub fn current_line(&self) -> &[Character] {
        let begin = self.row * self.dimensions.columns;
        let end = begin + self.dimensions.columns;

        &self.data[begin..end]
    }

    pub fn current_line_length(&self) -> usize {
        let line = self.current_line();

        line.iter().filter(|&a| a.character != char::default()).count()
    }

    pub fn move_right(&mut self) {
        self.column += 1;
        
        if self.column >= self.dimensions.columns {
            self.column = 0;
            self.row += 1;
        }
    }

    pub fn move_left(&mut self) {
        if self.column == 0 {
            self.row = self.row.saturating_sub(1);

            self.column = self.current_line_length();
        }
        
        self.column -= 1;
        
    }

    pub fn move_down(&mut self) {
        self.column = 0;
        self.row += 1;
    }

    pub fn move_to_beginning(&mut self) {
        self.column = 0;
    }

    fn previous_character(&self) -> &Character {
        let mut pos = self.data_position() - 1;

        while self.data[pos].character == char::default() {
            pos -= 1;
        }

        &self.data[pos]
    }

    pub fn print_char(&mut self, character: char) {
        let pos = self.data_position();

        if character == '\n' {
            self.move_down();
            return;
        } else if character as u32 == 8 {  // Backspace
            self.move_left();
            return;
        } else if character == '\r' {
            self.move_to_beginning();
            return;
        }

        if pos == 0 {
            self.data[pos] = Character {
                character,
                attribute: Attribute(0x00_ffffff),
            };
        } else {
            let prev_ch = self.previous_character();

            self.data[pos] = Character {
                character,
                attribute: prev_ch.attribute,
            };
        }

        self.move_right();
    }

    pub fn print_str(&mut self, input: &str) {
        for i in input.chars() {
            self.print_char(i);
        }
    }
}
