use noct_diskman::structures::Command;
use noct_tty::print;

use crate::println;

use super::ShellContext;

pub static DISKCTL_COMMAND_ENTRY: crate::ShellCommandEntry =
    ("diskctl", disk_ctl, Some("Control disk devices"));

pub enum DPMControlResult {
    Ok,
    Error(&'static str),
    MediaStatus(MediaStatus),
    HasData,
}

#[derive(Debug)]
pub enum MediaStatus {
    Offline,
    Loading,
    Online,
}

fn command_to_bin(command: &str) -> Option<Command> {
    match command {
        "eject" => Some(Command::Eject),
        "status" => Some(Command::GetMediumStatus),
        _ => None,
    }
}

// fn parse_reply(code: u32) -> Option<DPMControlResult> {
//     if code == 0 {
//         Some(DPMControlResult::Ok)
//     } else if code == 1 {
//         Some(DPMControlResult::HasData)
//     } else if code == DPM_ERROR_CANT_MOUNT as u32 {
//         Some(DPMControlResult::Error("Can't mount disk."))
//     } else if code == DPM_ERROR_NOT_IMPLEMENTED as u32 {
//         Some(DPMControlResult::Error("Feature not implemented."))
//     } else if code == DPM_ERROR_CANT_READ as u32 {
//         Some(DPMControlResult::Error("Can't read."))
//     } else if code == DPM_ERROR_NOT_READY as u32 {
//         Some(DPMControlResult::Error("Drive is not ready yet."))
//     } else if code == DPM_ERROR_NOT_ENOUGH as u32 {
//         Some(DPMControlResult::Error("Invalid buffer size."))
//     } else if code == DPM_ERROR_BUFFER as u32 {
//         Some(DPMControlResult::Error("Buffer error (passed NULL or data inside is invalid)"))
//     } else if (code & DPM_MEDIA_STATUS_MASK) == DPM_MEDIA_STATUS_MASK {
//         Some(DPMControlResult::MediaStatus({
//             let code = code & !DPM_MEDIA_STATUS_MASK;

//             if code == 0 {
//                 MediaStatus::Offline
//             } else if code == 1 {
//                 MediaStatus::Loading
//             } else if code == 2 {
//                 MediaStatus::Online
//             } else {
//                 unreachable!("You may have to implement other media statuses.")
//             }
//         }))
//     } else {
//         None
//     }
// }

pub fn show_help() {
    println!("Usage: diskctl disk_letter command");
    println!("Commands: eject, status");
}

pub fn disk_ctl(_context: &mut ShellContext, args: &[&str]) -> Result<(), usize> {
    let disk = match args.get(0) {
        Some(d) => *d,
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

    let mut output_buffer = [0u8; 256];

    let reply = noct_diskman::control(disk, command, &[], &mut output_buffer);

    println!("Reply: {reply}");

    for chunk in (&output_buffer).chunks(16) {
        for subchunk in chunk.chunks(4) {
            for value in subchunk {
                print!("{:02x} ", value);
            }

            print!(" ");
        }
        println!();
    }

    Ok(())
}
