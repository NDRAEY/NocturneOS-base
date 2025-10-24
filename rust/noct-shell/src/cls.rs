use super::ShellContext;
use noct_tty::c_api::tty_clear;

pub static CLS_COMMAND_ENTRY: crate::ShellCommandEntry = ("cls", cls, Some("Clear screen"));

pub fn cls(_context: &mut ShellContext, _args: &[&str]) -> Result<(), usize> {
    tty_clear();

    Ok(())
}
