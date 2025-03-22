use alloc::string::String;

use crate::println;

use super::ShellContext;

pub static CREATE_FILE_COMMAND_ENTRY: crate::shell::ShellCommandEntry =
    ("mkfile", create_file, Some("Makes a new file"));
pub static CREATE_DIR_COMMAND_ENTRY: crate::shell::ShellCommandEntry =
    ("mkdir", create_dir, Some("Makes a new directory"));
pub static REMOVE_FILE_COMMAND_ENTRY: crate::shell::ShellCommandEntry =
    ("rmfile", remove_file, Some("Removes a file"));
pub static REMOVE_DIR_COMMAND_ENTRY: crate::shell::ShellCommandEntry =
    ("rmdir", remove_dir, Some("Removes a directory"));

pub fn create_file(context: &mut ShellContext, args: &[String]) -> Result<(), usize> {
    if args.is_empty() {
        println!("Usage: mkfile <filename>");
        return Err(1);
    }

    let path = &args[0];

    let mut dir = context.current_path.clone();
    dir.apply(path);

    match noct_fileio::create_new_file(dir.as_str()) {
        Some(()) => Ok(()),
        None => Err(1),
    }
}

pub fn create_dir(context: &mut ShellContext, args: &[String]) -> Result<(), usize> {
    if args.is_empty() {
        println!("Usage: mkdir <filename>");
        return Err(1);
    }

    let path = &args[0];

    let mut dir = context.current_path.clone();
    dir.apply(path);

    match noct_fileio::create_new_directory(dir.as_str()) {
        Some(()) => Ok(()),
        None => Err(1),
    }
}

pub fn remove_file(context: &mut ShellContext, args: &[String]) -> Result<(), usize> {
    if args.is_empty() {
        println!("Usage: rmfile <filename>");
        return Err(1);
    }

    let path = &args[0];

    let mut dir = context.current_path.clone();
    dir.apply(path);

    match noct_fileio::remove_file(dir.as_str()) {
        Some(()) => Ok(()),
        None => Err(1),
    }
}

pub fn remove_dir(context: &mut ShellContext, args: &[String]) -> Result<(), usize> {
    if args.is_empty() {
        println!("Usage: rmdir <filename>");
        return Err(1);
    }

    let path = &args[0];

    let mut dir = context.current_path.clone();
    dir.apply(path);

    match noct_fileio::remove_directory(dir.as_str()) {
        Some(()) => Ok(()),
        None => Err(1),
    }
}
