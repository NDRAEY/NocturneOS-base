use alloc::{
    vec::Vec,
};

use noct_input::kbd::{Key, SpecialKey};
use noct_tty::println;

use noct_sys::scheduler::task_yield;

use super::ShellContext;

pub static LOG_COMMAND_ENTRY: crate::ShellCommandEntry =
    ("log", log, Some("Displays internal kernel logs"));

pub fn log(_context: &mut ShellContext, args: &[&str]) -> Result<(), usize> {
    if args.contains(&"-h") {
        println!("log - Displays kernel logs.\n");
        println!("Parameters:");
        println!("  -h  |  Displays this help");
        println!("  -w  |  Print all logs and wait for new. Pressing ESC will stop this process");

        return Ok(());
    }

    let is_continuous = args.contains(&"-w");

    for i in noct_il::get_logs().get() {
        println!("[{}] {}", i.time, i.message);
    }

    if is_continuous {
        let mut last_log_len = noct_il::get_logs().get().len();

        loop {
            let key = noct_input::keyboard_buffer_get_or_nothing();
            if let Some((key, _)) = noct_input::kbd::parse_scancode(key as _) {
                if key == Key::Special(SpecialKey::ESCAPE) {
                    break;
                }
            };

            let log_len = noct_il::get_logs().get().len();

            if log_len != last_log_len {
                let diff = log_len - last_log_len;

                let logger = noct_il::get_logs();
                let new_logs: Vec<_> = logger.get().iter().rev().take(diff).rev().collect();

                for i in new_logs {
                    println!("[{}] {}", i.time, i.message);
                }

                last_log_len = log_len;
            } else {
                task_yield();
            }
        }
    }

    Ok(())
}
