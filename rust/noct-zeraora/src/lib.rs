#![no_std]

use core::ffi::c_void;
use noct_logger::{qemu_err, qemu_note};

#[derive(Debug)]
pub struct Heap<'a> {
    pub metazone: &'a mut [HeapEntry],
    datazone_start_address: *mut c_void,
    used_entries: usize,
    total_entries: usize,
}

#[derive(Debug, Default, Copy, Clone)]
pub struct HeapEntry {
    address: usize,
    size: usize,
    is_used: bool,
}

pub fn align(value: usize, align: usize) -> usize {
    (value) + ((!value) & (align - 1))
}

impl Heap<'_> {
    pub fn with_entry_count(start_address: usize, size: usize, count: usize) -> Heap<'static> {
        let size_of_each_entry = core::mem::size_of::<HeapEntry>();
        let size_of_metazone = count * size_of_each_entry;

        // let datazone_start = start_address + size_of_metazone;
        let datazone_start = 0;

        qemu_note!("Size of each entry: {}", size_of_each_entry);
        qemu_note!("Size of metazone: {}", size_of_metazone);
        qemu_note!("Metazone start: {:x}", start_address);
        qemu_note!("Datazone start: {:x}", datazone_start);

        let pages = align(size_of_metazone, noct_physmem::PAGE_SIZE as usize) / noct_physmem::PAGE_SIZE as usize;

        qemu_note!("Will allocate: {} pages", pages);

        let mem = unsafe { noct_physmem::phys_alloc_multi_pages(pages as u32) };

        qemu_note!("Allocated pages at P{:x}", mem);

        unsafe {
            noct_physmem::map_pages(
                noct_physmem::get_kernel_page_directory(),
                mem as u32,
                start_address as u32,
                size_of_metazone as u32,
                noct_physmem::PAGE_WRITEABLE,
            )
        };

        let metazone =
            unsafe { core::slice::from_raw_parts_mut(start_address as *mut HeapEntry, count) };

        metazone.fill(HeapEntry::default());

        metazone[0] = HeapEntry {
            address: datazone_start,
            size: size,
            is_used: false,
        };

        Heap {
            metazone,
            datazone_start_address: datazone_start as *mut _,
            used_entries: 0,
            total_entries: count,
        }
    }

    pub fn shift_entries_to_right_by(&mut self, idx: usize, count: usize) {
        // Iterate backwards so we don't overwrite entries.
        for i in (idx..self.total_entries-count).rev() {
            self.metazone[i + count] = self.metazone[i];
        }
    }

    /// Splits the free block at `idx` into up to three parts:
    /// - a padding free block (if needed),
    /// - the allocated block of size `size` with given `alignment`, and
    /// - a remainder free block (if any space remains).
    ///
    /// Returns true on success, false if the block at `idx` cannot satisfy the allocation.
    pub fn split_idx(&mut self, size: usize, alignment: usize, idx: usize) -> Option<usize> {
        let block = self.metazone[idx];
        // Calculate the aligned start address within the block.
        let alloc_addr = align(block.address, alignment);
        let offset = alloc_addr - block.address;

        // Check if the free block is large enough (including padding).
        if block.size < offset + size {
            return None;
        }

        qemu_note!("{}", offset);

        // Two cases: no alignment offset vs. offset required.
        if offset == 0 {
            // No offset needed; behave like the simple split.
            if block.size == size {
                self.metazone[idx].is_used = true;
                return Some(idx);
            }
            // Split into allocated block and remainder.
            self.shift_entries_to_right(idx + 1);
            let remainder = block.size - size;
            self.metazone[idx + 1] = HeapEntry {
                address: block.address + size,
                size: remainder,
                is_used: false,
            };
            self.metazone[idx].size = size;
            self.metazone[idx].is_used = true;

            return Some(idx);
        } else {
            // We have a misaligned free block.
            // We will create:
            // 1. A padding block [block.address, offset) - remains free.
            // 2. The allocated block starting at aligned address with size `size`.
            // 3. Possibly a remainder free block if there's extra space.
            let remainder = block.size - offset - size;
            qemu_note!("Remainder: {}", remainder);
            if remainder > 0 {
                // Need to insert two extra entries: one for the allocated block and one for the remainder.
                // Shift entries starting at idx+1 by 2.
                self.shift_entries_to_right_by(idx + 1, 2);
                // Update current block to be the padding block.
                self.metazone[idx] = HeapEntry {
                    address: block.address,
                    size: offset,
                    is_used: false,
                };
                // Next entry is the allocated block.
                self.metazone[idx + 1] = HeapEntry {
                    address: block.address + offset + 1,
                    size: size,
                    is_used: true,
                };
                // Final entry is the remainder free block.
                self.metazone[idx + 2] = HeapEntry {
                    address: block.address + offset + size + 1,
                    size: remainder,
                    is_used: false,
                };
            } else {
                // No remainder block: we only need to insert one extra entry for the allocated block.
                // This happens if block.size == offset + size.
                self.shift_entries_to_right(idx + 1);
                self.metazone[idx] = HeapEntry {
                    address: block.address,
                    size: offset,
                    is_used: false,
                };
                self.metazone[idx + 1] = HeapEntry {
                    address: block.address + offset,
                    size: size,
                    is_used: true,
                };
            }
            return Some(idx + 1);
        }
    }

    // pub fn check(&self, size: usize, alignment: usize, idx: usize) -> bool {
    //     let block = self.metazone[idx];
    //     // Calculate the aligned start address within the block.
    //     let alloc_addr = align(block.address, alignment);
    //     let offset = alloc_addr - block.address;

    //     // Check if the free block is large enough (including padding).
    //     if block.size < offset + size {
    //         return false;
    //     }

    //     // Two cases: no alignment offset vs. offset required.
    //     if offset == 0 {
    //         // No offset needed; behave like the simple split.
    //         if block.size == size {
    //             return true;
    //         }

    //         return true;
    //     } else {
    //         return true;
    //     }
    // }

    pub fn alloc_no_map(&mut self, size: usize, mut alignment: usize) -> Option<usize> {
        if alignment == 0 {
            alignment = 1;
        }
        
        let len = self.metazone.len();

        for i in 0..len {
            qemu_note!("[{}; {}] {:?}", size, alignment, &self.metazone[i]);
            if self.metazone[i].is_used == false {
                if let Some(x) = self.split_idx(size, alignment, i) {
                    return Some(self.metazone[x].address);
                }
            }
        }

        None
    }

    pub fn free_no_unmap(&mut self, address: usize) -> Result<(), ()> {
        let len = self.metazone.len();

        for i in 0..len {
            qemu_note!("{} ? {}", self.metazone[i].address, address);
            if self.metazone[i].address == address {
                self.metazone[i].is_used = false;
                return Ok(());
            }
        }

        Err(())
    }

    pub fn shift_entries_to_right(&mut self, idx: usize) {
        for i in (idx..self.total_entries).rev() {
            self.metazone[i] = self.metazone[i - 1];
        }
    }

    pub fn shift_entries_to_left(&mut self, idx: usize) {
        for i in idx..self.total_entries {
            self.metazone[i] = self.metazone[i + 1];
        }
    }
}
