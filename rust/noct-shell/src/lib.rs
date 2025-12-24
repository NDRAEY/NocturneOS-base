#![no_std]

extern crate alloc;

use alloc::string::String;
use alloc::string::ToString;
use alloc::vec;
use alloc::vec::Vec;
use elf::endian::AnyEndian;
use noct_fs_sys::dir::Directory;
use noct_input::kbd::CharKey;
use noct_input::kbd::Key;
use noct_input::kbd::SpecialKey;
use noct_input::kbd::get_key;
use noct_logger::{qemu_err, qemu_note, qemu_warn};
use noct_sched::process_wait;
use noct_sched::spawn_prog_rust;
use noct_timer::timestamp;

use noct_sys::version::version;
use noct_tools::memory::memmeter;
use noct_tty::{print, println};

use noct_path::Path;

pub mod cat;
pub mod cd;
pub mod cls;
pub mod datetime;
pub mod dir;
pub mod disk_ctl;
pub mod disks;
pub mod file;
pub mod file_ops;
pub mod gfxinfo;
pub mod log;
pub mod mala;
pub mod meminfo;
pub mod mtrr;
pub mod pavi;
#[cfg(target_arch = "x86")]
pub mod pci;
pub mod reboot;
pub mod sysinfo;

pub type ShellCommand<E = usize> = fn(&mut ShellContext, &[&str]) -> Result<(), E>;
pub type ShellCommandEntry<'a, 'b> = (&'a str, ShellCommand, Option<&'b str>);

static COMMANDS: &[ShellCommandEntry] = &[
    dir::DIR_COMMAND_ENTRY,
    disks::DISKS_COMMAND_ENTRY,
    disk_ctl::DISKCTL_COMMAND_ENTRY,
    cls::CLS_COMMAND_ENTRY,
    cd::CD_COMMAND_ENTRY,
    file_ops::CREATE_DIR_COMMAND_ENTRY,
    file_ops::CREATE_FILE_COMMAND_ENTRY,
    file_ops::REMOVE_DIR_COMMAND_ENTRY,
    file_ops::REMOVE_FILE_COMMAND_ENTRY,
    file_ops::COPY_FILE_COMMAND_ENTRY,
    cat::CAT_COMMAND_ENTRY,
    meminfo::MEMINFO_COMMAND_ENTRY,
    #[cfg(target_arch = "x86")]
    pci::PCI_COMMAND_ENTRY,
    mala::MALA_COMMAND_ENTRY,
    mtrr::MTRR_COMMAND_ENTRY,
    pavi::PAVI_COMMAND_ENTRY,
    reboot::REBOOT_COMMAND_ENTRY,
    gfxinfo::GFXINFO_COMMAND_ENTRY,
    log::LOG_COMMAND_ENTRY,
    file::FILE_COMMAND_ENTRY,
    datetime::DATETIME_COMMAND_ENTRY,
    (
        "eni",
        |_, args| eni_player::player(args),
        Some("New player"),
    ),
    sysinfo::SYSINFO_COMMAND_ENTRY,
    ("help", help, Some("Prints help message")),
];

pub struct ShellContext {
    current_path: Path,
    command_history: Vec<String>,
}

impl ShellContext {
    fn new() -> Self {
        Self {
            current_path: Path::from_path(&noct_sched::me().cwd()).unwrap(),
            command_history: Vec::with_capacity(8),
        }
    }
}

fn help(_ctx: &mut ShellContext, _args: &[&str]) -> Result<(), usize> {
    for i in COMMANDS {
        println!("{:12} - {}", i.0, i.2.unwrap_or("No help at this moment"));
    }

    Ok(())
}

fn process_input(context: &mut ShellContext) -> String {
    let mut input = String::with_capacity(16);

    loop {
        let ch = get_key();

        match ch {
            CharKey::Key(key, pressed) => {
                if !pressed {
                    continue;
                }

                match key {
                    Key::Unknown => continue,
                    Key::Character(_) => continue,
                    Key::Special(SpecialKey::ENTER) => break,
                    Key::Special(SpecialKey::BACKSPACE) => {
                        if input.pop().is_some() {
                            print!("\x08 \x08");
                            noct_tty::c_api::tty_update();
                        }
                        continue;
                    }
                    Key::Special(SpecialKey::INSERT) => {
                        if let Some(command) = context.command_history.last() {
                            print!("{command}");
                            noct_tty::c_api::tty_update();
                            input.push_str(command);
                            break;
                        }
                    }
                    Key::Special(SpecialKey::TAB) => {
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

                        let (completions, completion_offset) = suggest_completions(stem);

                        qemu_note!("Completions: {:?}", completions);

                        if completions.is_empty() {
                            qemu_note!("No variants found.");
                        } else if completions.len() == 1 {
                            // Apply it
                            let completion = &completions[0];
                            let remnant = &completion[completion_offset.unwrap()..];

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

                        noct_tty::c_api::tty_update();

                        continue;
                    }
                    key => {
                        qemu_warn!("Unknown key: {:?}", key);
                    }
                }
            }
            CharKey::Char(ch) => {
                input.push(ch);

                print!("{}", ch);
                noct_tty::c_api::tty_update();
            }
        };
    }

    input
}

fn suggest_completions(stem: &str) -> (Vec<String>, Option<usize>) {
    if stem.is_empty() {
        return (vec![], None);
    }

    let is_absolute_path = stem.chars().next().is_some_and(|a| a.is_alphabetic())
        && stem.chars().nth(1).is_some_and(|a| a == ':')
        && stem.chars().nth(2).is_some_and(|a| a == '/');

    let fullpath = if !is_absolute_path {
        let mut path = Path::from_path(&noct_sched::me().cwd()).unwrap();

        path.apply(stem);

        path.into_string()
    } else {
        stem.to_string()
    };

    let mut path = Path::from_path(&fullpath).unwrap();

    if !fullpath.ends_with('/') {
        path.parent();
    }

    qemu_note!(
        "List all occurencies of `{}` in `{}`",
        fullpath,
        path.as_str()
    );

    let rdir = match Directory::from_path(&path) {
        Some(r) => r,
        None => {
            qemu_err!("Failed to build directory.");
            return (vec![], None);
        }
    };

    let variants = rdir.into_iter().names();

    let variants = variants
        .map(|name| path.as_string().clone() + &name)
        .filter(|full| full.starts_with(&fullpath))
        .collect();

    (variants, Some(fullpath.len()))
}

#[unsafe(no_mangle)]
pub fn new_nsh(_argc: u32, _argv: *const *const core::ffi::c_char) -> u32 {
    let mut context = ShellContext::new();

    let ver = version();

    println!("NocturneOS [Версия: v{}.{}.{}]", ver.0, ver.1, ver.2);
    println!("(c) SayoriOS & NocturneOS Team, 2025.");
    println!("Для дополнительной информации наберите \"help\".");

    loop {
        print!("{}> ", context.current_path.as_str());
        noct_tty::c_api::tty_update();

        let raw_input = process_input(&mut context);

        print!("\n");

        let timestart = timestamp();

        let memused = memmeter(|| process_command(&mut context, &raw_input));

        qemu_note!(
            "Memory delta: {} bytes (used {} bytes); Time: {}ms",
            memused.virtual_delta,
            memused.total_memory_run,
            timestamp() - timestart
        );

        context.command_history.push(raw_input);
    }

    // Should be `0` but infinite loop broke my plans.
}

fn parse_commandline(raw_input: &str) -> Vec<String> {
    let mut result = vec![];
    let mut current_command: String = String::new();
    let mut collecting_raw = false;

    for i in raw_input.chars() {
        if i == '\"' {
            collecting_raw = !collecting_raw;
            continue;
        }

        if !collecting_raw && i == ' ' {
            if !current_command.is_empty() {
                result.push(current_command.clone());
                current_command.clear();
            }

            continue;
        }

        current_command.push(i);
    }

    if !current_command.is_empty() {
        result.push(current_command.clone());
        current_command.clear();
    }

    result
}

fn process_command(context: &mut ShellContext, raw_input: &str) {
    let com = parse_commandline(raw_input);

    if com.is_empty() {
        return;
    }

    let (command, arguments) = (&com[0], if com.len() > 1 { &com[1..] } else { &[] });

    let object = COMMANDS.iter().filter(|x| x.0 == command).next_back();

    match object {
        Some(descriptor) => {
            let args: Vec<&str> = arguments.iter().map(|a| a.as_str()).collect();
            let status = descriptor.1(context, &args);

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

            qemu_note!("{path:?}");
            let file = noct_fs::File::open(&path);

            match file {
                Ok(mut file) => {
                    let mut data = [0u8; 512];
                    file.read(&mut data).unwrap();

                    match elf::file::parse_ident::<AnyEndian>(&data) {
                        Ok(_) => {
                            let args: Vec<&str> = arguments.iter().map(|a| a.as_str()).collect();

                            let pid = spawn_prog_rust(&path, &args);

                            qemu_note!("Started PID {pid}");

                            unsafe { process_wait(pid as _) };

                            qemu_note!("Program finished!");
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

    // println!("Command: `{}`, Arguments: {:?}", command, arguments);
}
