use alloc::string::ToString;

use noct_logger::qemu_log;
use noct_tty::{print, println};

use super::ShellContext;

pub static CAT_COMMAND_ENTRY: crate::ShellCommandEntry = ("cat", cat, Some("Prints out a file"));

pub fn cat(context: &mut ShellContext, args: &[&str]) -> Result<(), usize> {
    if args.len() == 0 {
        println!("Usage: cat <file>");
        return Err(1);
    }

    for filepath in args {
        let mut fullpath = context.current_path.clone();
        fullpath.apply(filepath);

        let path = fullpath.as_str();

        let file = noct_fs::read_to_string(path);

        match file {
            Ok(data) => {
                print!("{}", data);
            }
            Err(e) => {
                println!("Error: {e} ({})", path);
                return Err(1);
            }
        };
    }

    Ok(())
}
