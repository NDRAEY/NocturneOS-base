use noct_tty::println;

use crate::system::mem;

pub struct MeterResults {
    pub virtual_delta: usize,
    pub total_memory_run: usize,
}

pub fn memmeter(mut f: impl FnMut()) -> MeterResults {
    let virt = mem::get_stats().used_virtual;
    let trun = mem::get_stats().total_memory_run;

    f();

    let end_virt = mem::get_stats().used_virtual;
    let end_trun = mem::get_stats().total_memory_run;

    MeterResults {
        virtual_delta: end_virt - virt,
        total_memory_run: end_trun - trun,
    }
}
