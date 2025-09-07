use alloc::string::String;

use crate::{println, system::mem};

use super::ShellContext;

pub static MTRR_COMMAND_ENTRY: crate::shell::ShellCommandEntry =
    ("mtrr", mtrr, Some("MTRR info"));

unsafe extern "C" {
    fn list_mtrrs();
}

pub fn mtrr(_context: &mut ShellContext, _args: &[&str]) -> Result<(), usize> {
    unsafe { list_mtrrs() };

    Ok(())
}
