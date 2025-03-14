use alloc::string::{String, ToString};

use crate::{print, println};

use super::ShellContext;

pub static PD_COMMAND_ENTRY: crate::shell::ShellCommandEntry = ("desktop", desktop, Some("Run Parallel Desktop."));

extern "C" {
    fn parallel_desktop_start();
}

pub fn desktop(context: &mut ShellContext, args: &[String]) -> Result<(), usize> {
    unsafe { parallel_desktop_start() };

    Ok(())
}
