use core::result;

pub mod input;
pub mod rgb_image;
pub mod screen;

pub use noct_logger::*;

pub type Result<T> = result::Result<T, ()>;
