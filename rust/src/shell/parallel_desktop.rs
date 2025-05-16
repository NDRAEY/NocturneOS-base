use super::ShellContext;
use alloc::string::String;

pub static PD_COMMAND_ENTRY: crate::shell::ShellCommandEntry =
    ("desktop", desktop, Some("Run Parallel Desktop."));

extern "C" {
    fn parallel_desktop_start();
}

pub fn desktop(_context: &mut ShellContext, _args: &[String]) -> Result<(), usize> {
    unsafe { parallel_desktop_start() };

    Ok(())
}
