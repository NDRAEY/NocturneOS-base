use noct_mem::get_stats;

pub struct MeterResults {
    pub virtual_delta: usize,
    pub total_memory_run: usize,
}

pub fn memmeter(mut f: impl FnMut()) -> MeterResults {
    let mut stats = get_stats();

    let virt = stats.used_virtual;
    let trun = stats.total_memory_run;

    f();

    stats = get_stats();

    let end_virt = stats.used_virtual;
    let end_trun = stats.total_memory_run;

    MeterResults {
        virtual_delta: end_virt - virt,
        total_memory_run: end_trun - trun,
    }
}
