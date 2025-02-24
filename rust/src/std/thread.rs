use crate::qemu_log;

#[repr(C)]
pub struct ListItem {
    prev: *const u32,
    next: *const u32,
    list: *const u32,
}

#[repr(C)]
pub struct Process {
    list_item: ListItem,
    page_dir: usize,
    threads_count: usize,
    suspend: usize,
    pid: usize,
    page_dir_virt: usize,
    name: [u8; 256],
    page_tables_virts: [usize; 1024],
}

#[repr(C)]
pub struct Thread {
    list_item: ListItem,
    process: *const Process,
    suspend: u32,
    stack_size: usize,
    stack: *const usize,
    esp: usize,
    entry_point: usize,
    id: usize,
    stack_top: usize,
    eax: usize,
    ebx: usize,
    ecx: usize,
    edx: usize,
    esi: usize,
    edi: usize,
    ebp: usize,
    state: u32,
}

extern "C" {
    fn get_current_proc() -> *mut Process;
    fn thread_create(
        proc: *mut Process,
        entry_point: usize,
        stack_size: usize,
        kernel: bool,
        suspend: bool,
    ) -> *mut Thread;
}

pub fn spawn(f: fn() -> ()) -> *mut Thread {
    let addr = f as usize;

    qemu_log!("Func: {:x}", addr);

    unsafe {
        let proc = get_current_proc();
        qemu_log!("Proc: {:?}", proc);

        thread_create(
            proc,
            addr, 
            64 << 10,
            true,
            false)
    }
    // f();
}
