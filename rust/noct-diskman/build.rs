use std::{env, io::{Seek, Write}};

pub const DISKMAN_COMMANDS: &[(&str, u32)] = &[
    ("EJECT", 0x00),
    ("GET_MEDIUM_STATUS", 0x01),
    ("GET_DRIVE_TYPE", 0x02),
    ("GET_MEDIUM_CAPACITY", 0x03),
];

fn write_diskman_commands_header() {
    let mut file = std::fs::OpenOptions::new().write(true).create(true).open("../../kernel/include/generated/diskman_commands.h").unwrap();

    file.write(b"#pragma once\n\n").unwrap();

    for (command, code) in DISKMAN_COMMANDS {
        file.write(format!("#define DISKMAN_COMMAND_{command} ({code})\n").as_bytes()).unwrap();
    }

    let pos = file.stream_position().unwrap();
    file.set_len(pos).unwrap();
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

    write_diskman_commands_header();
}