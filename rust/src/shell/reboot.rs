use alloc::string::String;

use super::ShellContext;

pub static REBOOT_COMMAND_ENTRY: crate::shell::ShellCommandEntry =
    ("reboot", reboot_w, Some("Restarts the system"));

extern "C" {
    fn reboot();
}

pub fn reboot_w(_context: &mut ShellContext, _args: &[String]) -> Result<(), usize> {
    unsafe { reboot() };

    Ok(())
}
