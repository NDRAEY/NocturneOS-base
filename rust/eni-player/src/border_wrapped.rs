use embedded_graphics::{
    Drawable,
    prelude::{Dimensions, DrawTarget, Point},
    primitives::{PrimitiveStyle, Rectangle, StyledDrawable},
};
use embedded_layout::View;

pub struct BorderWrapped<T>
where
    T: Drawable + Dimensions,
{
    drawable: T,

    border_color: T::Color,
    border_width: usize,
}

impl<T> BorderWrapped<T>
where
    T: Drawable + Dimensions,
    T::Color: Default,
{
    pub fn new(drawable: T) -> Self {
        Self {
            drawable,
            border_color: T::Color::default(),
            border_width: 0,
        }
    }

    pub fn with_border_color(mut self, color: T::Color) -> Self {
        self.border_color = color;
        self
    }

    pub fn with_border_width(mut self, width: usize) -> Self {
        self.border_width = width;
        self
    }

    pub fn drawable(&self) -> &T {
        &self.drawable
    }

    pub fn drawable_mut(&mut self) -> &mut T {
        &mut self.drawable
    }
}

impl<T> Drawable for BorderWrapped<T>
where
    T: Drawable + Dimensions,
{
    type Color = T::Color;
    type Output = T::Output;

    fn draw<D>(&self, target: &mut D) -> Result<T::Output, D::Error>
    where
        D: DrawTarget<Color = Self::Color>,
    {
        let pos = self.drawable.bounding_box().top_left;

        let value = self.drawable.draw(target)?;

        Rectangle::new(pos, self.drawable.bounding_box().size()).draw_styled(
            &PrimitiveStyle::with_stroke(self.border_color, self.border_width as _),
            target,
        )?;

        Ok(value)
    }
}
