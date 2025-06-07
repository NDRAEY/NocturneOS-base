use core::{cell::OnceCell, ffi::{c_char, CStr}};

use lazy_static::lazy_static;
use noct_logger::qemu_note;
use spin::Mutex;

use crate::{console::Console, renderer::{RenderedConsole}};

lazy_static! {
    static ref CONSOLE: Mutex<OnceCell<RenderedConsole>> = Mutex::new(OnceCell::new());
}

pub fn init() {
    let dimensions = noct_screen::dimensions();

    let columns = dimensions.0 / 8;
    let rows = dimensions.1 / 16;

    CONSOLE.lock().get_or_init(|| RenderedConsole::new(Console::new(rows, columns)));
}

#[unsafe(no_mangle)]
pub extern "C" fn tty_init() {
    init();
}

pub fn tty_puts_str(s: &str) {
    let mut binding= CONSOLE.lock();
    let console = binding.get_mut().unwrap();
    
    console.print_str(s);
}

#[unsafe(no_mangle)]
pub extern "C" fn tty_puts(s: *const c_char) {
    let s = unsafe { CStr::from_ptr(s) };
    let s = s.to_str().unwrap();

    tty_puts_str(s);
}

#[unsafe(no_mangle)]
pub extern "C" fn tty_putchar(c: c_char) {
    let mut binding= CONSOLE.lock();
    let console = binding.get_mut().unwrap();

    console.console_mut().print_char(char::from(c as u8));
    console.render(None);
}

#[unsafe(no_mangle)]
pub extern "C" fn tty_clear() {
    let mut binding= CONSOLE.lock();
    let console = binding.get_mut().unwrap();

    console.console_mut().clear();
    console.console_mut().set_position(0, 0);
    unsafe {
        noct_screen::clean_screen();
    }
    console.render(None);
}

#[unsafe(no_mangle)]
pub extern "C" fn tty_get_pos_x() -> u32 {
    let binding= CONSOLE.lock();
    let console = binding.get().unwrap();

    console.console().position().0 as u32
}

#[unsafe(no_mangle)]
pub extern "C" fn tty_get_pos_y() -> u32 {
    let binding= CONSOLE.lock();
    let console = binding.get().unwrap();

    console.console().position().1 as u32
}

#[unsafe(no_mangle)]
pub extern "C" fn tty_update() {
    let mut binding= CONSOLE.lock();
    let console = binding.get_mut().unwrap();

    unsafe {
        noct_screen::clean_screen();
    }

    console.render(None);
}