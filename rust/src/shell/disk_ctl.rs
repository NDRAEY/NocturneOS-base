use alloc::string::String;
use noct_dpm_sys::{
    DPM_COMMAND_EJECT, DPM_COMMAND_GET_STATUS, DPM_ERROR_CANT_MOUNT, DPM_ERROR_CANT_READ,
    DPM_ERROR_NOT_IMPLEMENTED, DPM_ERROR_NOT_READY, DPM_STATUS_CHECK_DEBUG_CONSOLE,
};

use crate::{println, system::mem};

use super::ShellContext;

pub static DISKCTL_COMMAND_ENTRY: crate::shell::ShellCommandEntry =
    ("diskctl", disk_ctl, Some("Control disk devices"));

fn command_to_bin(command: &str) -> Option<u32> {
    match command {
        "eject" => Some(DPM_COMMAND_EJECT),
        "status" => Some(DPM_COMMAND_GET_STATUS),
        _ => None,
    }
}

fn parse_reply(code: u32) -> Option<&'static str> {
    if code == 0 {
        Some("ok")
    } else if code == DPM_ERROR_CANT_MOUNT as u32 {
        Some("Can't mount disk.")
    } else if code == DPM_ERROR_NOT_IMPLEMENTED as u32 {
        Some("Feature not implemented.")
    } else if code == DPM_ERROR_CANT_READ as u32 {
        Some("Can't read.")
    } else if code == DPM_ERROR_NOT_READY as u32 {
        Some("Drive is not ready yet.")
    } else if code == DPM_STATUS_CHECK_DEBUG_CONSOLE as u32 {
        Some("Check debug console!")
    } else {
        None
    }
}

pub fn show_help() {
    println!("Usage: diskctl disk_letter command");
    println!("Commands: eject, status");
}

pub fn disk_ctl(_context: &mut ShellContext, args: &[&str]) -> Result<(), usize> {
    let disk = match args.get(0) {
        Some(d) => match d.chars().nth(0) {
            Some(d) => d.to_ascii_uppercase(),
            None => {
                println!("Invalid disk: {d:?}");
                show_help();

                return Err(1);
            }
        },
        None => {
            println!("No disk specified!");
            show_help();

            return Err(1);
        }
    };

    let command = match args.get(1) {
        Some(c) => match command_to_bin(*c) {
            Some(c) => c,
            None => {
                println!("Invalid command: {c}");
                show_help();

                return Err(1);
            }
        },
        None => {
            println!("No command specified!");
            show_help();

            return Err(1);
        }
    };

    let reply = unsafe { noct_dpm_sys::dpm_ctl(disk as u32 as _, command, core::ptr::null(), 0) };

    match parse_reply(reply) {
        Some(r) => {
            println!("Reply: {}", r);
        }
        None => {
            println!("Unknown reply: {reply:x}");
        }
    }

    Ok(())
}
