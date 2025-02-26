use alloc::string::{String, ToString};
use alloc::vec;
use alloc::vec::Vec;

use crate::{print, println};
use crate::std::io::input::getchar;
use crate::std::io::screen::screen_update;

use noct_path::Path;

struct ShellContext {
    current_path: Path
}

impl ShellContext {
    fn new() -> Self {
        Self {
            current_path: Path::from_path("R:/").unwrap()
        }
    }
}

fn process_input() -> String {
    let mut input = String::with_capacity(16);

    loop {
        let raw_ch = unsafe { getchar() };
        let ch = char::from_u32(raw_ch).unwrap();

        if ch == '\0' {
            continue;
        }

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
pub fn new_nsh(_argc: u32, _argv: *const *const core::ffi::c_char) -> u32 {
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

    // Should be `0` but infinite loop broke my plans.
}

fn parse_commandline(raw_input: String) -> Vec<String> {
    let mut result = vec![];
    let mut current_command: String = String::new();
    let mut collecting_raw = false;

    for i in raw_input.chars() {
        if !collecting_raw && i == ' ' {
            if !current_command.is_empty() {
                result.push(current_command.clone());
                current_command.clear();
            }
            
            continue;
        }

        if i == '\"' {
            collecting_raw = !collecting_raw;
        }

        current_command.push(i);
    }

    if !current_command.is_empty() {
        result.push(current_command.clone());
        current_command.clear();
    }

    result
}

fn process_command(context: &mut ShellContext, raw_input: String) {
    let com = parse_commandline(raw_input);

    println!("{:?}", com);
}
