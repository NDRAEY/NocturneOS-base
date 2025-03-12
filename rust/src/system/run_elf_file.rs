use alloc::{string::{String, ToString}, vec::Vec};

extern "C" {
    // int32_t run_elf_file(const char *name, int argc, char* argv[]);

    pub fn run_elf_file(name: *const u8, argc: i32, argv: *const *const u8) -> i32;
}

pub fn run(name: &str, args: &[String]) -> i32 {
    let argc = args.len();

    let mut cloned_name = name.to_string();
    cloned_name.push('\0');

    // Copy strings
    let mut vec_args: Vec<String> = Vec::with_capacity(argc);
    vec_args.extend_from_slice(args);

    let argv_ptrs = vec_args.iter().map(|arg| arg.as_ptr()).collect::<Vec<*const u8>>();
    let argv_ptrs_ptr = argv_ptrs.as_ptr();

    unsafe { run_elf_file(cloned_name.as_ptr(), argc as i32, argv_ptrs_ptr) }
}