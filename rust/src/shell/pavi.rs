use core::ffi::c_char;

use alloc::{string::String, vec::Vec};
use super::ShellContext;

pub static PAVI_COMMAND_ENTRY: crate::shell::ShellCommandEntry = ("pavi", pavi, Some("The Image Viewer."));

extern "C" {
    fn pavi_view(argc: u32, args: *const *const c_char);
}

pub fn pavi(_context: &mut ShellContext, _args: &[String]) -> Result<(), usize> {
    let mut args = Vec::from(_args);
    args.insert(0, String::from("pavi"));

    let ptrs: Vec<*const u8> = args.iter().map(|x| x.as_str().as_ptr()).collect();
    let ptr = ptrs.as_ptr();

    unsafe { pavi_view(ptrs.len() as _, ptr as *const *const _) };

    Ok(())
}
