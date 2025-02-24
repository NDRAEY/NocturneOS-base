use crate::println;

#[no_mangle]
pub fn new_nsh(argc: u32, argv: *const *const core::ffi::c_char) -> u32 {
    println!("Hello, world!");
    0
}
