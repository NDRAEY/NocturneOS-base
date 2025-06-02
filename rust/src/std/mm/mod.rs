use noct_tty::println;

use crate::system::mem;

pub fn memmeter(mut f: impl FnMut()) -> usize {
    let data = mem::get_stats().used_virtual;

    f();

    let data2 = mem::get_stats().used_virtual;

    data2 - data
}