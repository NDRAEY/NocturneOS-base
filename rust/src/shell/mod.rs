use alloc::borrow::ToOwned;
use alloc::string::String;
use alloc::vec;
use alloc::vec::Vec;
use elf::endian::AnyEndian;
use noct_logger::qemu_err;

use crate::std::io::input::getchar;
use crate::std::io::screen::screen_update;
use crate::{print, println};

use noct_path::Path;

pub mod cd;
pub mod cls;
pub mod dir;

pub type ShellCommand<E = usize> = fn(&mut ShellContext, &[String]) -> Result<(), E>;
pub type ShellCommandEntry<'a, 'b> = (&'a str, ShellCommand, Option<&'b str>);

static COMMANDS: &[ShellCommandEntry] = &[
    dir::DIR_COMMAND_ENTRY,
    cls::CLS_COMMAND_ENTRY,
    cd::CD_COMMAND_ENTRY,
    ("help", help, Some("Prints help message")),
];

pub struct ShellContext {
    current_path: Path,
}

impl ShellContext {
    fn new() -> Self {
        Self {
            current_path: Path::from_path("R:/").unwrap(),
        }
    }
}

fn help(_ctx: &mut ShellContext, _args: &[String]) -> Result<(), usize> {
    for i in COMMANDS {
        println!("{:12} - {}", i.0, i.2.unwrap_or("No help at this moment"));
    }

    Ok(())
}

fn process_input() -> String {
    let mut input = String::with_capacity(16);

    loop {
        let raw_ch = unsafe { getchar() };
        let ch = char::from_u32(raw_ch).unwrap();

        if ch == '\0' {
            continue;
        }

        if ch == '\n' {
            break;
        }

        if ch as u32 == 8 {
            if input.pop().is_some() {
                print!("{0} {0}", char::from_u32(8).unwrap());
            }
            continue;
        }

        input.push(ch);

        print!("{}", ch);
        unsafe { screen_update() };
    }

    input
}

#[no_mangle]
pub fn new_nsh(_argc: u32, _argv: *const *const core::ffi::c_char) -> u32 {
    let mut context = ShellContext::new();

    println!("nsh v0.1.0\n");
    unsafe { screen_update() };

    loop {
        print!("{}> ", context.current_path.as_str());
        unsafe { screen_update() };

        let raw_input = process_input();

        print!("\n");

        process_command(&mut context, raw_input);
    }

    // Should be `0` but infinite loop broke my plans.
}

fn parse_commandline(raw_input: &str) -> Vec<String> {
    let mut result = vec![];
    let mut current_command: String = String::new();
    let mut collecting_raw = false;

    for i in raw_input.chars() {
        if !collecting_raw && i == ' ' {
            if !current_command.is_empty() {
                result.push(current_command.clone());
                current_command.clear();
            }

            continue;
        }

        if i == '\"' {
            collecting_raw = !collecting_raw;
        }

        current_command.push(i);
    }

    if !current_command.is_empty() {
        result.push(current_command.clone());
        current_command.clear();
    }

    result
}

fn process_command(context: &mut ShellContext, raw_input: String) {
    let com = parse_commandline(&raw_input);

    if com.is_empty() {
        return;
    }

    let (command, arguments) = (&com[0], if com.len() > 1 { &com[1..] } else { &[] });

    let object = COMMANDS.iter().filter(|x| x.0 == command).last();

    match object {
        Some(descriptor) => {
            let status = descriptor.1(context, arguments);

            if let Err(err) = status {
                qemu_err!(
                    "Command `{:?}` did not exited successfully!!! (code: {})",
                    command,
                    err
                );
            }
        }
        None => {
            let path = context.current_path.as_string().clone() + command;
            let file = crate::std::fs::File::open(&path);

            match file {
                Ok(mut file) => {
                    let mut data = [0u8; 512];
                    file.read(&mut data).unwrap();

                    match elf::file::parse_ident::<AnyEndian>(&data) {
                        Ok(_) => {
                            todo!("Run ELF file");
                        }
                        Err(_) => {
                            println!("{}: not an ELF file", path);
                        }
                    }
                }
                Err(_error) => {
                    println!("{}: command not found", command);
                }
            }
        }
    }

    println!("Command: `{}`, Arguments: {:?}", command, arguments);
}
