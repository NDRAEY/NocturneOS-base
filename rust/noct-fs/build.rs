use std::{env, path::PathBuf};

use bindgen::RustTarget;

fn main() {
    let bindings = bindgen::Builder::default()
        .header("../../kernel/include/lib/stdio.h")
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
}
