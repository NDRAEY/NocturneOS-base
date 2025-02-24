use alloc::string::String;

use crate::{print, println};
use crate::std::io::input::getchar;
use crate::std::io::screen::screen_update;

fn process_input() -> String {
    let mut input = String::with_capacity(16);

    loop {
        let raw_ch = unsafe { getchar() };
        let ch = char::from_u32(raw_ch).unwrap();

        if ch == '\n' {
            break;
        }

        if ch as u32 == 8 {
            if input.pop().is_some() {
                print!("{0} {0}", char::from_u32(8).unwrap());
            }
            continue;
        }
                
        input.push(ch);
        
        print!("{}", ch);
        unsafe { screen_update() };
    }

    input
}

#[no_mangle]
pub fn new_nsh(argc: u32, argv: *const *const core::ffi::c_char) -> u32 {
    println!("Hello, world!");
    unsafe { screen_update() };

    loop {
        print!("> ");
        unsafe { screen_update() };

        let raw_input = process_input();

        print!("\n");
        println!("`{}`", raw_input);
    }

    0
}
