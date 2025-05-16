use alloc::string::{String, ToString};

use noct_tty::{print, println};

use super::ShellContext;

pub static CAT_COMMAND_ENTRY: crate::shell::ShellCommandEntry =
    ("cat", cat, Some("Prints out a file"));

pub fn cat(context: &mut ShellContext, args: &[String]) -> Result<(), usize> {
    let filepath = &args[0];

    let mut fullpath = context.current_path.clone();
    fullpath.apply(filepath);

    let path = fullpath.as_str().to_string();

    let file = noct_fs::read_to_string(&path);

    match file {
        Ok(data) => {
            print!("{}", data);
        }
        Err(e) => {
            println!("Error: {e} ({})", path);
            return Err(1);
        }
    };

    Ok(())
}
