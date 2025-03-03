use alloc::string::{String, ToString};
use noct_fs_sys::FSM_TYPE_DIR;

use crate::println;

use super::ShellContext;

pub static DIR_COMMAND_ENTRY: crate::shell::ShellCommandEntry = ("dir", dir, Some("Lists a directory"));

pub fn dir(context: &mut ShellContext, args: &[String]) -> Result<(), usize> {
    let path = {
        match args.first() {
            Some(elem) => elem.clone(),
            None => {
                context.current_path.as_str().to_string()
            },
        }
    };

    let dir = noct_fs_sys::dir::Directory::from_path(&path);

    if dir.is_none() {
        println!("`{}` read error", path);
        return Err(1);
    }

    let dir = dir.unwrap();

    println!("Listing directory: `{}`\n", path);

    for file in dir {
        let fdatetime = file.file.LastTime;
        let ftype = file.file.Type;
        let fsize = file.file.Size;

        println!("{} [{:4}] [{:8} bytes]\t{}", fdatetime.format(), {
            if ftype as u32 == FSM_TYPE_DIR { "DIR" } else { "FILE" }
        }, fsize, file.name);
    }

    Ok(())
}
