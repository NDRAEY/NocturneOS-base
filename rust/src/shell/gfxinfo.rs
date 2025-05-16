use alloc::string::String;
use noct_tty::println;

use super::{ShellCommandEntry, ShellContext};

pub static GFXINFO_COMMAND_ENTRY: ShellCommandEntry = ("gfxinfo", gfxinfo, None);

pub fn gfxinfo(_ctx: &mut ShellContext, _argv: &[String]) -> Result<(), usize> {
    let (width, height) = noct_screen::dimensions();
    let pitch = noct_screen::pitch();
    let bpp = noct_screen::bits_per_pixel();

    let framebuffer = noct_screen::framebuffer().as_ptr().addr();
    let back_framebuffer = noct_screen::back_framebuffer().as_ptr().addr();

    println!("Screen resolution: {width}x{height}");
    println!("Pitch: {pitch}");
    println!("Bits per pixel: {bpp}");
    println!("Framebuffer at: 0x{framebuffer:x}");
    println!("Back framebuffer at: 0x{back_framebuffer:x}");

    Ok(())
}
