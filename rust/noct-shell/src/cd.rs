use noct_fs_sys::dir::Directory;

use noct_logger::qemu_note;
use noct_sys::chdir_nonrelative;

use crate::println;

use super::ShellContext;

pub static CD_COMMAND_ENTRY: crate::ShellCommandEntry = ("cd", cd, Some("Change directory"));

pub fn cd(context: &mut ShellContext, args: &[&str]) -> Result<(), usize> {
    let path = {
        match args.first() {
            Some(elem) => elem,
            None => ".",
        }
    };

    let mut dir = context.current_path.clone();
    dir.apply(path);

    if !Directory::is_accessible(dir.as_str()) {
        println!("Cannot access: `{}`", dir);

        return Err(1);
    }

    context.current_path = dir;

    chdir_nonrelative(context.current_path.as_str());

    Ok(())
}
