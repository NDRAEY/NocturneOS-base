use std::{env, path::PathBuf};

use bindgen::RustTarget;

fn main() {
    // I allow to use deprecated `RustTarget::Stable_1_77` because it's only
    // the method to get rid of `error: extern block cannot be declared unsafe` error
    
    #[allow(deprecated)]
    let bindings = bindgen::Builder::default()
        .header("../../kernel/include/mem/pmm.h")
        .clang_arg("-I../../kernel/include/")
        .parse_callbacks(Box::new(bindgen::CargoCallbacks::new()))
        .use_core()
        .size_t_is_usize(false)
        .layout_tests(false)
        .generate_comments(false)
        .rust_target(RustTarget::Stable_1_77)
        .generate()
        .expect("Binding generation error");

    let out = PathBuf::from(env::var("OUT_DIR").unwrap());

    bindings.write_to_file(out.join("bindings.rs"))
        .expect("Could not write result to bindings.rs");
}
