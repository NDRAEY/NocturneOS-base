use crate::println;

use super::ShellContext;

pub static DATETIME_COMMAND_ENTRY: crate::ShellCommandEntry =
    ("date", date, Some("Shows current date and time"));

pub fn date(_context: &mut ShellContext, _args: &[&str]) -> Result<(), usize> {
    let time = unsafe { noct_time::get_time() };

    println!(
        "{:02}/{:02}/{:02} {:02}:{:02}:{:02}",
        time.day, time.month, time.year, time.hours, time.minutes, time.seconds
    );

    Ok(())
}
