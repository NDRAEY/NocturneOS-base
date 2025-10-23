use core::str::FromStr;

use alloc::string::String;
use noct_fs_sys::dir::Directory;

use crate::{println, system::chdir_nonrelative};

use super::ShellContext;

pub static CD_COMMAND_ENTRY: crate::shell::ShellCommandEntry = ("cd", cd, Some("Change directory"));

pub fn cd(context: &mut ShellContext, args: &[&str]) -> Result<(), usize> {
    let path = {
        match args.first() {
            Some(elem) => elem,
            None => ".",
        }
    };

    let mut dir = context.current_path.clone();
    dir.apply(path);

    if !Directory::is_accessible(&dir) {
        println!("Cannot access: `{}`", dir);

        return Err(1);
    }

    context.current_path = dir;

    chdir_nonrelative(context.current_path.as_str());

    Ok(())
}
