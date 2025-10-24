use crate::{println};

use super::ShellContext;

pub static MEMINFO_COMMAND_ENTRY: crate::ShellCommandEntry =
    ("meminfo", meminfo, Some("Memory info"));

pub fn meminfo(_context: &mut ShellContext, _args: &[&str]) -> Result<(), usize> {
    let data = noct_mem::get_stats();

    let p_used = data.used_physical;
    let p_free = data.free_physical();
    let v_used = data.used_virtual;
    let v_count = data.heap_allocated_count;
    let h_pk_u = data.peak_heap_usage;
    let tmr = data.total_memory_run;

    println!("Физическая:");
    println!(
        "    Используется: {} байт ({} KB | {} MB)",
        p_used,
        p_used >> 10,
        p_used >> 20
    );
    println!(
        "    Свободно: {} байт ({} KB | {} MB)",
        p_free,
        p_free >> 10,
        p_free >> 20
    );
    println!();
    println!("Виртуальная:");
    println!("    {} записей", v_count);
    println!(
        "    Используется: {} байт ({} KB | {} MB)",
        v_used,
        v_used >> 10,
        v_used >> 20
    );
    println!(
        "    Пиковое потребление: {} байт ({} KB | {} MB)",
        h_pk_u,
        h_pk_u >> 10,
        h_pk_u >> 20
    );
    println!(
        "    Выделено за всё время работы системы: {} байт ({} KB | {} MB)",
        tmr,
        tmr >> 10,
        tmr >> 20
    );

    Ok(())
}
