use super::ShellContext;

use pavi::pavi as pavi_view;

pub static PAVI_COMMAND_ENTRY: crate::ShellCommandEntry =
    ("pavi", pavi, Some("The Image Viewer."));

pub fn pavi(_context: &mut ShellContext, args: &[&str]) -> Result<(), usize> {
    pavi_view(args)
}
