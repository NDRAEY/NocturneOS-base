use alloc::format;
use noct_tty::println;

use super::ShellContext;

pub static DISKS_COMMAND_ENTRY: crate::ShellCommandEntry =
    ("disks", disks, Some("Show all disks in DPM"));

pub fn disks(_ctx: &mut ShellContext, _args: &[&str]) -> Result<(), usize> {
    for (n, i) in noct_diskman::disk_list().iter().enumerate() {
        println!(
            "{:-2}. {:-7} -> {:?} ({} sectors, {} bytes each; {})",
            n + 1,
            i.id,
            i.name,
            {
                if let Some(cap) = i.capacity {
                    &format!("{cap}")
                } else {
                    &format!("------")
                }
            },
            {
                if let Some(blk) = i.block_size {
                    &format!("{blk}")
                } else {
                    &format!("-----")
                }
            },
            {
                if i.is_partition {
                    &format!("{:?} [Partition]", i.drive_type)
                } else {
                    &format!("{:?}", i.drive_type)
                }
            }
        );
    }

    Ok(())
}
