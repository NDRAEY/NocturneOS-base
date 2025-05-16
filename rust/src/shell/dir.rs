use alloc::string::{String, ToString};
use noct_fs_sys::{dir::Directory, FSM_ENTITY_TYPE_TYPE_DIR};

use crate::println;

use super::ShellContext;

pub static DIR_COMMAND_ENTRY: crate::shell::ShellCommandEntry =
    ("dir", dir, Some("Lists a directory"));

pub fn dir(context: &mut ShellContext, args: &[String]) -> Result<(), usize> {
    let path = {
        match args.first() {
            Some(elem) => elem.clone(),
            None => context.current_path.as_str().to_string(),
        }
    };

    let dir = match Directory::from_path(&path) {
        Some(x) => x,
        None => {
            println!("`{}`: no such directory", path);
            return Err(1);
        }
    };

    println!("Listing directory: `{}`\n", path);

    for file in &dir {
        let fdatetime = file.file.LastTime;
        let ftype = file.file.Type;
        let fsize = file.file.Size;

        println!(
            "{} [{:4}] [{:8} bytes]\t{}",
            fdatetime.format(),
            {
                if ftype == FSM_ENTITY_TYPE_TYPE_DIR {
                    "DIR"
                } else {
                    "FILE"
                }
            },
            fsize,
            file.name
        );
    }

    Ok(())
}
