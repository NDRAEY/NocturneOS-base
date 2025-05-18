use core::ffi::CStr;

use alloc::ffi::CString;
use noct_fs_sys::{fsm_info, nvfs_info};
use noct_tty::println;

use super::{ShellCommandEntry, ShellContext};

pub static FILE_COMMAND_ENTRY: ShellCommandEntry = ("file", file, Some("Shows info about file"));

pub fn file(_ctx: &mut ShellContext, args: &[&str]) -> Result<(), usize> {
    if args.is_empty() {
        println!("No arguments!");
        return Err(1);
    }
    
    let filepath = args[0];

    let rawfp = CString::new(filepath).unwrap();
    let rawfp: *mut i8 = rawfp.into_raw();

    let file = unsafe { nvfs_info(rawfp) };

    core::mem::drop(unsafe { CString::from_raw(rawfp) });

    println!("File {filepath:?}\n");

    println!("Date: {}", file.LastTime.format());
    println!("Size: {}", { let x = file.Size; x });
    println!("Mode: {:08x}", { let x = file.Mode; x });

    Ok(())
}