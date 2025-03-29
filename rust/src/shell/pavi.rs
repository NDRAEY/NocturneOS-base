use core::ffi::c_char;

use alloc::{string::String, vec::Vec};
use super::ShellContext;

use pavi::pavi as pavi_view;

pub static PAVI_COMMAND_ENTRY: crate::shell::ShellCommandEntry = ("pavi", pavi, Some("The Image Viewer."));

pub fn pavi(_context: &mut ShellContext, _args: &[String]) -> Result<(), usize> {
    let mut args = Vec::from(_args);
    args.insert(0, String::from("pavi"));

    pavi_view(args.len() as _, &args)
}
