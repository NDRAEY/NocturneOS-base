#![no_std]

extern crate alloc;

pub mod console;
pub mod renderer;
pub mod c_api;

use core::fmt;
use core::fmt::Write;
use lazy_static::lazy_static;
use spin::Mutex;

#[macro_export]
macro_rules! print {
    ($($arg:tt)*) => ($crate::_print_tty(format_args!($($arg)*)));
}

#[macro_export]
macro_rules! println {
    () => ($crate::print!("\n"));
    ($($arg:tt)*) => ($crate::print!("{}\n", format_args!($($arg)*)));
}

#[doc(hidden)]
pub fn _print_tty(args: fmt::Arguments) {
    WRITER.lock().write_fmt(args).unwrap();
}

lazy_static! {
    pub static ref WRITER: Mutex<Writer> = Mutex::new(Writer);
}

pub struct Writer;
impl fmt::Write for Writer {
    fn write_str(&mut self, s: &str) -> fmt::Result {
        c_api::tty_puts_str(s);
        Ok(())
    }
}
