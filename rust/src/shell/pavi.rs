use super::ShellContext;
use alloc::{string::String, vec::Vec};

use pavi::pavi as pavi_view;

pub static PAVI_COMMAND_ENTRY: crate::shell::ShellCommandEntry =
    ("pavi", pavi, Some("The Image Viewer."));

pub fn pavi(_context: &mut ShellContext, args: &[String]) -> Result<(), usize> {
    pavi_view(&args)
}
