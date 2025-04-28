use core::result;

pub mod input;
pub mod screen;

pub use noct_logger::*;

pub type Result<T> = result::Result<T, ()>;
