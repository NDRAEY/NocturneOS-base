use alloc::string::String;

use super::ShellContext;

extern "C" {
    pub fn clean_tty_screen();
}

pub static CLS_COMMAND_ENTRY: crate::shell::ShellCommandEntry = ("cls", cls, Some("Clear screen"));

pub fn cls(_context: &mut ShellContext, _args: &[&str]) -> Result<(), usize> {
    unsafe {
        clean_tty_screen();
    }

    Ok(())
}
