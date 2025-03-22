use core::ffi::c_char;

use alloc::{string::String, vec::Vec};
use super::ShellContext;

pub static MALA_COMMAND_ENTRY: crate::shell::ShellCommandEntry = ("mala", mala, Some("A drawing program"));

extern "C" {
    fn mala_draw(argc: u32, argv: *const *const c_char) -> u32;
}

pub fn mala(_context: &mut ShellContext, _args: &[String]) -> Result<(), usize> {
    let mut args = Vec::from(_args);
    args.insert(0, String::from("mala"));

    let ptrs: Vec<*const u8> = args.iter().map(|x| x.as_str().as_ptr()).collect();
    let ptr = ptrs.as_ptr();

    unsafe { mala_draw(0, ptr as *const *const _) };

    Ok(())
}
