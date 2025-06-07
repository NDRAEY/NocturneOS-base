use alloc::string::String;

use super::ShellContext;
use crate::tty::tty_clear;

pub static CLS_COMMAND_ENTRY: crate::shell::ShellCommandEntry = ("cls", cls, Some("Clear screen"));

pub fn cls(_context: &mut ShellContext, _args: &[&str]) -> Result<(), usize> {
    tty_clear();

    Ok(())
}
