use alloc::string::String;
use alloc::vec;
use alloc::vec::Vec;
use elf::endian::AnyEndian;
use noct_fs_sys::dir::Directory;
use noct_logger::{qemu_err, qemu_note, qemu_warn};

use crate::std::io::input::getchar;
use crate::std::io::screen::screen_update;
use crate::system::run_elf_file::run;
use crate::system::version::version;
use noct_tty::{print, println};

use noct_path::Path;

pub mod cat;
pub mod cd;
pub mod cls;
pub mod dir;
pub mod file_ops;
pub mod mala;
pub mod meminfo;
pub mod miniplay;
pub mod parallel_desktop;
pub mod pavi;
pub mod pci;
pub mod reboot;

pub type ShellCommand<E = usize> = fn(&mut ShellContext, &[String]) -> Result<(), E>;
pub type ShellCommandEntry<'a, 'b> = (&'a str, ShellCommand, Option<&'b str>);

static COMMANDS: &[ShellCommandEntry] = &[
    dir::DIR_COMMAND_ENTRY,
    cls::CLS_COMMAND_ENTRY,
    cd::CD_COMMAND_ENTRY,
    file_ops::CREATE_DIR_COMMAND_ENTRY,
    file_ops::CREATE_FILE_COMMAND_ENTRY,
    file_ops::REMOVE_DIR_COMMAND_ENTRY,
    file_ops::REMOVE_FILE_COMMAND_ENTRY,
    cat::CAT_COMMAND_ENTRY,
    parallel_desktop::PD_COMMAND_ENTRY,
    meminfo::MEMINFO_COMMAND_ENTRY,
    pci::PCI_COMMAND_ENTRY,
    mala::MALA_COMMAND_ENTRY,
    pavi::PAVI_COMMAND_ENTRY,
    miniplay::MINIPLAY_COMMAND_ENTRY,
    reboot::REBOOT_COMMAND_ENTRY,
    ("m", miniplay::mp, None),
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

fn process_input(context: &mut ShellContext) -> String {
    let mut input = String::with_capacity(16);

    loop {
        let raw_ch = unsafe { getchar() };
        let ch = char::from_u32(raw_ch).unwrap();

        qemu_note!("Key is: {}", raw_ch);

        if ch == '\0' {
            continue;
        }

        if ch == '\n' {
            break;
        }

        // Backspace
        if ch as u32 == 8 {
            if input.pop().is_some() {
                print!("{0} {0}", char::from_u32(8).unwrap());
            }
            continue;
        } else if ch == '\t' {
            // Tab
            qemu_warn!("Tab!");

            if input.trim().is_empty() {
                continue;
            }

            let mut back_counter = input.len();

            while back_counter > 0 && input.chars().nth(back_counter) != Some(' ') {
                back_counter -= 1;
            }

            qemu_warn!("Fin");

            let stem = input.as_str()[back_counter..].trim();

            qemu_note!("Finding completions for: `{}`", stem);

            let completions = suggest_completions(stem);

            qemu_note!("Completions: {:?}", completions);

            if completions.is_empty() {
                qemu_note!("No variants found.");
            } else if completions.len() == 1 {
                // Apply it
                let completion = &completions[0];
                let remnant = &completion[stem.len()..];

                input.push_str(remnant);

                print!("{}", remnant);
            } else {
                // Show variants

                println!("\n- Available variants:");
                
                for i in &completions {
                    print!("{i} ");
                }

                println!();

                print!("{}> {input}", context.current_path.as_str());
            }

            continue;
        }

        input.push(ch);

        print!("{}", ch);
        unsafe { screen_update() };
    }

    input
}

fn suggest_completions(stem: &str) -> Vec<String> {
    if stem.is_empty() {
        return vec![];
    }

    if stem.chars().next().is_some_and(|a| a.is_alphabetic())
        && stem.chars().nth(1).is_some_and(|a| a == ':')
        && stem.chars().nth(2).is_some_and(|a| a == '/')
    {
        // It's path

        let mut path = Path::from_path(stem).unwrap();

        path.parent();

        qemu_note!("List all occurencies of `{}` in `{}`", stem, path.as_str());

        let rdir = Directory::from_path(&path).unwrap();
        let variants = rdir.into_iter().names();

        return variants
            .into_iter()
            .map(|name| path.as_string().clone() + &name)
            .filter(|full| full.starts_with(stem))
            .collect();
    }

    vec![]
}

#[no_mangle]
pub fn new_nsh(_argc: u32, _argv: *const *const core::ffi::c_char) -> u32 {
    let mut context = ShellContext::new();

    let ver = version();

    println!("NocturneOS [Версия: v{}.{}.{}]", ver.0, ver.1, ver.2);
    println!("(c) SayoriOS & NocturneOS Team, 2025.");
    println!("Для дополнительной информации наберите \"help\".");

    unsafe { screen_update() };

    loop {
        print!("{}> ", context.current_path.as_str());
        unsafe { screen_update() };

        let raw_input = process_input(&mut context);

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

    let object = COMMANDS.iter().filter(|x| x.0 == command).next_back();

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
            let file = noct_fs::File::open(&path);

            match file {
                Ok(mut file) => {
                    let mut data = [0u8; 512];
                    file.read(&mut data).unwrap();

                    match elf::file::parse_ident::<AnyEndian>(&data) {
                        Ok(_) => {
                            // todo!("Run ELF file");
                            run(path.as_str(), &com);
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
