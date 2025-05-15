use alloc::string::String;
use noct_dpm;
use noct_tty::println;

use super::ShellContext;

pub static DISKS_COMMAND_ENTRY: crate::shell::ShellCommandEntry = ("disks", disks, Some("Show all disks in DPM"));

pub fn disks(_ctx: &mut ShellContext, _argv: &[String]) -> Result<(), usize> {
    for i in noct_dpm::Disks::new() {
        println!("{}: {:?} - {:?} ({} sectors, {} bytes each)", i.letter, i.name, i.filesystem, i.sector_count, i.sector_size)
    }

    Ok(())
}