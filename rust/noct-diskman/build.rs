use std::{env, io::Write};

pub const DISKMAN_COMMANDS: &[(&str, u32)] = &[
    // Eject the medium.
    //
    // No command parameters supplied.
    //
    // No return data expected.
    ("EJECT", 0x00),
    // Get medium status.
    //
    // No command parameters supplied.
    //
    // Returns 4 bytes representing an `u32` as MediumStatus.
    ("GET_MEDIUM_STATUS", 0x01),
    // Get drive type.
    //
    // No command parameters supplied.
    //
    // Returns 4 bytes representing an `u32` as DriveType.
    ("GET_DRIVE_TYPE", 0x02),
    // Get medium capacity.
    //
    // No command parameters supplied.
    //
    // Returns 12 bytes:
    // - bytes `0..8` representing an `u64` as actual capacity in sectors.
    // - bytes `8..12` representing an `u32` as block size.
    ("GET_MEDIUM_CAPACITY", 0x03),
];

pub const DISKMAN_DRIVE_TYPES: &[(&str, u32)] = &[
    ("HARD_DRIVE", 0x00),
    ("OPTICAL_DRIVE", 0x01),
    ("REMOVABLE_MEDIA", 0x02),
    ("UNKNOWN", 0xff),
];

pub const DISKMAN_MEDIUM_STATUSES: &[(&str, u32)] =
    &[("OFFLINE", 0x00), ("LOADING", 0x01), ("ONLINE", 0x02)];

fn write_diskman_c_header() {
    let mut file = std::fs::OpenOptions::new()
        .write(true)
        .create(true)
        .truncate(true)
        .open("../../kernel/include/generated/diskman_commands.h")
        .unwrap();

    file.write_all(b"#pragma once\n\n").unwrap();

    for (command, code) in DISKMAN_COMMANDS {
        file.write_all(format!("#define DISKMAN_COMMAND_{command} ({code})\n").as_bytes())
            .unwrap();
    }

    for (dtype, code) in DISKMAN_DRIVE_TYPES {
        file.write_all(format!("#define DISKMAN_TYPE_{dtype} ({code})\n").as_bytes())
            .unwrap();
    }

    for (mstatus, code) in DISKMAN_MEDIUM_STATUSES {
        file.write_all(format!("#define DISKMAN_MEDIUM_{mstatus} ({code})\n").as_bytes())
            .unwrap();
    }
}

fn main() {
    let crate_dir = env::var("CARGO_MANIFEST_DIR").unwrap();

    cbindgen::Builder::new()
        .with_crate(crate_dir)
        .with_language(cbindgen::Language::C)
        .with_no_includes()
        .with_include_guard("NOCTURNE_DISK_MANAGER")
        .generate()
        .expect("Unable to generate bindings")
        .write_to_file("../../kernel/include/generated/diskman.h");

    write_diskman_c_header();
}
