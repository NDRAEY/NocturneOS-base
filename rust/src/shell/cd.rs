use core::str::FromStr;

use alloc::string::String;
use noct_fs_sys::dir::Directory;

use crate::println;

use super::ShellContext;

pub static CD_COMMAND_ENTRY: crate::shell::ShellCommandEntry = ("cd", cd, Some("Change directory"));

pub fn cd(context: &mut ShellContext, args: &[String]) -> Result<(), usize> {
    let path = {
        match args.first() {
            Some(elem) => elem.clone(),
            None => {
                String::from_str(".").unwrap()
            },
        }
    };

    let mut dir = context.current_path.clone();
    dir.apply(&path);
    
    if !Directory::is_accessible(&dir) {
        println!("Cannot access: `{}`", dir.as_str());

        return Err(1);
    }

    context.current_path = dir;

    Ok(())
}
