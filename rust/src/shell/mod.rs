use alloc::string::String;
use alloc::vec::{self, Vec};

use crate::{print, println};
use crate::std::io::input::getchar;
use crate::std::io::screen::screen_update;

struct ShellContext {
    current_path: String
}

impl ShellContext {
    fn new() -> Self {
        Self {
            current_path: "R:\\".to_string()
        }
    }
}

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
    let mut context = ShellContext::new();

    println!("Hello, world!");
    unsafe { screen_update() };

    loop {
        print!("> ");
        unsafe { screen_update() };

        let raw_input = process_input();

        print!("\n");

        process_command(&mut context, raw_input);
    }

    0
}

fn parse_commandline(raw_input: String) -> Vec<String> {
    let mut result = vec![];
    let mut current_command: String = String::new();

    for i in &raw_input {
        // ...
    }

    result
}


fn process_command(context: &mut ShellContext, raw_input: String) {
    
}
