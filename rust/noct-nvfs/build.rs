use std::{env, path::PathBuf};

use bindgen::RustTarget;

fn main() {
    println!("cargo:rerun-if-changed=../../kernel/include/fs/nvfs.h");
    
    let bindings = bindgen::Builder::default()
        .header("../../kernel/include/fs/nvfs.h")
        .clang_arg("-I../../kernel/include/")
        .parse_callbacks(Box::new(bindgen::CargoCallbacks::new()))
        .use_core()
        .size_t_is_usize(false)
        .layout_tests(false)
        .generate_comments(false)
        .rust_target(RustTarget::default())
        .generate()
        .expect("Binding generation error");

    let out = PathBuf::from(env::var("OUT_DIR").unwrap());

    bindings
        .write_to_file(out.join("bindings.rs"))
        .expect("Could not write result to bindings.rs");

    let crate_dir = env::var("CARGO_MANIFEST_DIR").unwrap();

    cbindgen::Builder::new()
        .with_crate(crate_dir)
        .with_language(cbindgen::Language::C)
        .with_no_includes()
        .with_include_guard("NOCTURNE_NVFS")
        .generate()
        .expect("Unable to generate bindings")
        .write_to_file("../../kernel/include/generated/nvfs_helper.h");
}