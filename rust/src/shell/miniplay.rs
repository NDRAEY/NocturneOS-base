use core::ffi::c_char;

use alloc::{string::String, vec::Vec};
use super::ShellContext;

pub static MINIPLAY_COMMAND_ENTRY: crate::shell::ShellCommandEntry = ("miniplay", miniplay_w, Some("Media player."));

extern "C" {
    fn miniplay(argc: u32, argv: *const *const c_char);
}

pub fn miniplay_w(_context: &mut ShellContext, _args: &[String]) -> Result<(), usize> {
    let mut args = Vec::from(_args);
    args.insert(0, String::from("miniplay"));

    let ptrs: Vec<*const u8> = args.iter().map(|x| x.as_str().as_ptr()).collect();
    let ptr = ptrs.as_ptr();

    unsafe { miniplay(ptrs.len() as _, ptr as *const *const _) };

    Ok(())
}
