pub struct MemoryInfo {
    pub total_physical: usize,
    pub used_physical: usize,
    pub heap_allocated_count: usize,
    pub used_virtual: usize,
    pub peak_heap_usage: usize
}

impl MemoryInfo {
    #[inline]
    pub fn free_physical(&self) -> usize {
        self.total_physical - self.used_physical
    }
}

extern "C" {
    static used_phys_memory_size: usize;
    static phys_memory_size: usize;
    static peak_heap_usage: usize;

    fn heap_allocated_count() -> usize;
    fn heap_used_memory() -> usize;
}

pub fn get_stats() -> MemoryInfo {
    MemoryInfo {
        total_physical: unsafe { phys_memory_size } as _,
        used_physical: unsafe { used_phys_memory_size } as _,
        heap_allocated_count: unsafe { heap_allocated_count() },
        used_virtual: unsafe { heap_used_memory() },
        peak_heap_usage: unsafe { peak_heap_usage as _ }
    }
}
