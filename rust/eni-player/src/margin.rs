use embedded_graphics::{
    Drawable,
    prelude::{Dimensions, DrawTarget, Point, Size, Transform},
    primitives::Rectangle,
};

use embedded_graphics::draw_target::DrawTargetExt;

#[derive(Clone, Default)]
pub struct MarginValue {
    top: u32,
    bottom: u32,
    left: u32,
    right: u32,
}

#[derive(Clone)]
pub struct Margin<T>
where
    T: Drawable + Dimensions,
{
    drawable: T,

    value: MarginValue,
}

impl<T> Margin<T>
where
    T: Drawable + Dimensions,
{
    pub fn new(drawable: T) -> Self {
        Self {
            drawable,
            value: MarginValue::default(),
        }
    }

    pub const fn with_margin(mut self, value: MarginValue) -> Self {
        self.value = value;
        self
    }

    pub const fn with_all_constraints(mut self, value: u32) -> Self {
        self.value = MarginValue {
            top: value,
            bottom: value,
            left: value,
            right: value,
        };
        self
    }

    pub const fn bottom(mut self, value: u32) -> Self {
        self.value.bottom = value;
        self
    }

    pub const fn top(mut self, value: u32) -> Self {
        self.value.top = value;
        self
    }
    
    pub const fn left(mut self, value: u32) -> Self {
        self.value.left = value;
        self
    }

    pub const fn right(mut self, value: u32) -> Self {
        self.value.right = value;
        self
    }

    pub const fn drawable(&self) -> &T {
        &self.drawable
    }

    pub const fn drawable_mut(&mut self) -> &mut T {
        &mut self.drawable
    }
}

impl<T> Drawable for Margin<T>
where
    T: Drawable + Dimensions,
{
    type Color = T::Color;
    type Output = T::Output;

    fn draw<D>(&self, target: &mut D) -> Result<Self::Output, D::Error>
    where
        D: DrawTarget<Color = Self::Color>,
    {
        let mut ed = target.translated(Point::new(self.value.left as _, self.value.top as _));

        self.drawable.draw(&mut ed)
    }
}

impl<T> Dimensions for Margin<T>
where
    T: Drawable + Dimensions,
{
    fn bounding_box(&self) -> Rectangle {
        let original = self.drawable.bounding_box();

        let size = Size::new(
            original.size.width + self.value.right + self.value.left,
            original.size.height + self.value.bottom + self.value.top,
        );

        Rectangle::new(original.top_left, size)
    }
}

impl<T> Transform for Margin<T> where T: Drawable + Dimensions + Clone + Transform {
    fn translate(&self, by: Point) -> Self {
        let mut new = Self::clone(self);

        new.drawable = new.drawable.translate(by);

        new
    }

    fn translate_mut(&mut self, by: Point) -> &mut Self {
        self.drawable.translate_mut(by);
        self
    }
}
