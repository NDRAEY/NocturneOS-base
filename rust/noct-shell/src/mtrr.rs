use super::ShellContext;

pub static MTRR_COMMAND_ENTRY: crate::ShellCommandEntry =
    ("mtrr", mtrr, Some("MTRR info"));

unsafe extern "C" {
    fn list_mtrrs();
}

pub fn mtrr(_context: &mut ShellContext, _args: &[&str]) -> Result<(), usize> {
    unsafe { list_mtrrs() };

    Ok(())
}
