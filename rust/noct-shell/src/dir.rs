use alloc::vec::Vec;
use noct_fs_sys::{FSM_ENTITY_TYPE_TYPE_DIR, dir::Directory, file::File};

use crate::println;

use super::ShellContext;

pub static DIR_COMMAND_ENTRY: crate::ShellCommandEntry = ("dir", dir, Some("Lists a directory"));

// Sorts entities of dir by name (A-Z) ascending by using bubble sort.
fn sort_entities(files: &mut [File]) {
    for _ in 0..files.len() {
        for n in 0..files.len() {
            if n + 1 >= files.len() {
                break;
            }

            let this = files.get(n).unwrap();
            let next = files.get(n + 1).unwrap();

            if this.name > next.name {
                files.swap(n, n + 1);
            }
        }
    }
}

pub fn dir(context: &mut ShellContext, args: &[&str]) -> Result<(), usize> {
    let path = {
        match args.first() {
            Some(elem) => elem,
            None => context.current_path.as_str(),
        }
    };

    let dir = match Directory::from_path(path) {
        Some(x) => x,
        None => {
            println!("`{}`: no such directory", path);
            return Err(1);
        }
    };

    println!("Listing directory: `{}`\n", path);

    let mut files: Vec<File> = dir.into_iter().collect();

    sort_entities(&mut files);

    for file in files {
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
