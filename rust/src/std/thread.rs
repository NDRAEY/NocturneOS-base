use noct_sched::{get_current_proc, thread_create, thread_t};

pub fn spawn(f: fn() -> ()) -> *mut thread_t {
    let addr = f as usize;

    // qemu_log!("Func: {:x}", addr);

    unsafe {
        let proc = get_current_proc();
        // qemu_log!("Proc: {:?}", proc);

        thread_create(
            proc,
            addr as _, 
            64 << 10,
            true,
            false)
    }
    // f();
}
