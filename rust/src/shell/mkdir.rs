use alloc::string::String;
use noct_fs_sys::dir::Directory;

use crate::println;

use super::ShellContext;
use crate::std::fs::make_directory;

pub static MKDIR_COMMAND_ENTRY: crate::shell::ShellCommandEntry = ("mkdir", mkdir, Some("Makes a new directory"));

pub fn mkdir(context: &mut ShellContext, args: &[String]) -> Result<(), usize> {
    let path = &args[0];

    let mut dir = context.current_path.clone();
    dir.apply(&path);
    
    if Directory::is_accessible(&dir) {
        println!("Already exists: `{}`", dir.as_str());

        return Err(1);
    }

    match make_directory(&path) {
        Ok(_) => {
            println!("Complete!");
            Ok(())
        },
        Err(_) => {
            println!("Error making directory: `{}`", path);
            Err(1)
        }
    }
}
