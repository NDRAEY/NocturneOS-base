use core::ffi::c_char;

use super::ShellContext;
use alloc::{
    borrow::ToOwned, string::{String, ToString}, vec::Vec
};

pub static MINIPLAY_COMMAND_ENTRY: crate::shell::ShellCommandEntry =
    ("miniplay", miniplay_w, Some("Media player."));

extern "C" {
    fn miniplay(argc: u32, argv: *const *const c_char);
}

pub fn miniplay_w(_context: &mut ShellContext, args: &[&str]) -> Result<(), usize> {
    let mut args: Vec<String> = args.iter().map(|a| a.to_string()).collect();
    args.insert(0, "miniplay".to_owned());

    for i in &mut args {
        i.push('\0');
    }

    let ptrs: Vec<*const u8> = args.iter().map(|x| x.as_str().as_ptr()).collect();
    let ptr = ptrs.as_ptr();

    unsafe { miniplay(ptrs.len() as _, ptr as *const *const _) };

    Ok(())
}

pub fn mp(_context: &mut ShellContext, _args: &[&str]) -> Result<(), usize> {
    miniplay_w(_context, &["R:/test_sound.wav"])
}
