use core::cmp;

use alloc::{string::{String, ToString}, vec::Vec};
use embedded_graphics::{
    Drawable,
    prelude::{Dimensions, DrawTarget, PixelColor, Point, Size},
    primitives::{PrimitiveStyle, PrimitiveStyleBuilder, Rectangle, StyledDrawable},
    text::{self, Baseline, Text, renderer::TextRenderer},
};
use embedded_layout::View;

pub struct TextList<TR: TextRenderer> {
    elements: Vec<String>,

    default_text_style: Option<TR>,
    selected_text_style: Option<TR>,
    selected_highlight_color: TR::Color,

    selected_index: usize,

    position: Point,
}

impl<TR> TextList<TR>
where
    TR: TextRenderer + Clone,
    TR::Color: Default,
{
    pub const fn new() -> Self {
        Self {
            elements: Vec::new(),
            selected_index: 0,
            selected_highlight_color: TR::Color::default(),
            position: Point::zero(),

            default_text_style: None,
            selected_text_style: None,
        }
    }

    pub fn with_default_style(mut self, text_style: TR) -> Self {
        self.default_text_style = Some(text_style.clone());
        self.selected_text_style = Some(text_style);
        self
    }

    pub fn with_selected_style(mut self, text_style: TR) -> Self {
        self.selected_text_style = Some(text_style);
        self
    }

    pub fn with_position(mut self, position: Point) -> Self {
        self.position = position;
        self
    }

    pub fn with_highlight_color(mut self, color: TR::Color) -> Self {
        self.selected_highlight_color = color;
        self
    }
}

impl<TR> TextList<TR>
where
    TR: TextRenderer,
{
    pub fn add<T: ToString>(&mut self, entry: T) {
        self.elements.push(entry.to_string());
    }

    pub fn get(&self, index: usize) -> Option<&String> {
        self.elements.get(index)
    }

    pub fn get_mut(&mut self, index: usize) -> Option<&mut String> {
        self.elements.get_mut(index)
    }

    pub fn remove(&mut self, index: usize) {
        self.elements.remove(index);
    }

    pub const fn elements(&self) -> &Vec<String> {
        &self.elements
    }

    pub const fn elements_mut(&mut self) -> &mut Vec<String> {
        &mut self.elements
    }

    pub const fn move_down(&mut self) {
        self.selected_index = (self.selected_index + 1).clamp(0, self.elements.len());
    }

    pub const fn move_up(&mut self) {
        if self.selected_index == 0 {
            return;
        }

        self.selected_index = (self.selected_index - 1).clamp(0, self.elements.len());
    }
}

impl<TR> Drawable for TextList<TR>
where
    TR: TextRenderer + Clone,
{
    type Color = TR::Color;
    type Output = Point;

    fn draw<Drw>(&self, target: &mut Drw) -> Result<Point, Drw::Error>
    where
        Drw: DrawTarget<Color = Self::Color>,
    {
        let mut next_position = self.position;

        let mut x = self.position.x;
        let mut y = self.position.y;

        for (n, i) in self.elements.iter().enumerate() {
            let text = if n == self.selected_index {
                Text::with_baseline(
                    i,
                    Point::new(x, y),
                    self.selected_text_style.clone().unwrap(),
                    Baseline::Top,
                )
            } else {
                Text::with_baseline(
                    i,
                    Point::new(x, y),
                    self.default_text_style.clone().unwrap(),
                    Baseline::Top,
                )
            };

            let size = text.size();

            if n == self.selected_index {
                let whole_width = self.bounding_box().size.width;

                Rectangle::new(Point::new(x, y), Size::new(whole_width, size.height)).draw_styled(
                    &PrimitiveStyleBuilder::new()
                        .fill_color(self.selected_highlight_color)
                        .build(),
                    target,
                )?;
            }

            y += size.height as i32;

            next_position = text.draw(target)?;
        }

        Ok(next_position)
    }
}

impl<TR> Dimensions for TextList<TR>
where
    TR: TextRenderer + Clone,
{
    fn bounding_box(&self) -> Rectangle {
        let mut width = 0;
        let mut height = 0;

        for i in self.elements.iter() {
            let text = Text::with_baseline(
                i,
                Point::new(width, height),
                // &self.default_text_style,     // Doesn't work :(
                self.default_text_style.clone().unwrap(),
                Baseline::Top,
            );

            let size = text.size();

            width = cmp::max(width, size.width as i32);
            height += size.height as i32;
        }

        Rectangle::new(self.position, Size::new(width as _, height as _))
    }
}
