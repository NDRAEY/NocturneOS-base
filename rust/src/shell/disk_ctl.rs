use alloc::string::String;
use noct_dpm_sys::{
    DPM_COMMAND_EJECT, DPM_COMMAND_GET_MEDIUM_STATUS, DPM_ERROR_CANT_MOUNT, DPM_ERROR_CANT_READ,
    DPM_ERROR_NOT_IMPLEMENTED, DPM_ERROR_NOT_READY
};

use crate::{println, system::mem};

use super::ShellContext;

pub static DISKCTL_COMMAND_ENTRY: crate::shell::ShellCommandEntry =
    ("diskctl", disk_ctl, Some("Control disk devices"));

pub enum DPMControlResult {
    Error(&'static str),
    Boolean(bool)
}

fn command_to_bin(command: &str) -> Option<u32> {
    match command {
        "eject" => Some(DPM_COMMAND_EJECT),
        "status" => Some(DPM_COMMAND_GET_MEDIUM_STATUS),
        _ => None,
    }
}

fn parse_reply(code: u32) -> Option<DPMControlResult> {
    if code == 0 {
        Some(DPMControlResult::Error("ok"))
    } else if code == DPM_ERROR_CANT_MOUNT as u32 {
        Some(DPMControlResult::Error("Can't mount disk."))
    } else if code == DPM_ERROR_NOT_IMPLEMENTED as u32 {
        Some(DPMControlResult::Error("Feature not implemented."))
    } else if code == DPM_ERROR_CANT_READ as u32 {
        Some(DPMControlResult::Error("Can't read."))
    } else if code == DPM_ERROR_NOT_READY as u32 {
        Some(DPMControlResult::Error("Drive is not ready yet."))
    } else if (code & 0xC000_0000) == 0xC000_0000 {
        Some(DPMControlResult::Boolean((code & !0xC000_0000) != 0))
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
            match r {
                DPMControlResult::Error(e) => {
                    println!("Drive error: {}", e);
                },
                DPMControlResult::Boolean(b) => {
                    println!("Drive responsed with boolean: {}", b);
                },
            }
        }
        None => {
            println!("Unknown reply: {reply:x}");
        }
    }

    Ok(())
}
