use std::env;

fn main() {
    let crate_dir = env::var("CARGO_MANIFEST_DIR").unwrap();

    cbindgen::Builder::new()
        .with_crate(crate_dir)
        .with_language(cbindgen::Language::C)
        .with_no_includes()
        .with_include("common.h")
        .with_include_guard("NOCTURNE_TTY")
        .generate()
        .expect("Unable to generate bindings")
        .write_to_file("../../kernel/include/generated/tty_headers.h");
}
