use alloc::format;
use noct_tty::println;

use super::ShellContext;

pub static DISKS_COMMAND_ENTRY: crate::ShellCommandEntry =
    ("disks", disks, Some("Show all disks in DPM"));

pub fn disks(_ctx: &mut ShellContext, _args: &[&str]) -> Result<(), usize> {
    for (n, i) in noct_diskman::disk_list().iter().enumerate() {
        let capacity = match i.capacity {
            Some(cap) => format!("{cap}"),
            None => "-".repeat(7)
        };

        let block_size = match i.block_size {
            Some(blk) => format!("{blk}"),
            None => "-".repeat(7)
        };

        let tags = if i.is_partition {
            format!("{:?} [Partition]", i.drive_type)
        } else {
            format!("{:?}", i.drive_type)
        };

        println!(
            "{:-2}. {:-7} -> {:?} ({} sectors, {} bytes each; {})",
            n + 1,
            i.id,
            i.name,
            capacity,
            block_size,
            tags
        );
    }

    Ok(())
}
