#![no_std]

extern crate alloc;

use alloc::{string::String, vec::Vec};
use elf::{ParseError, endian::AnyEndian};
use noct_logger::{qemu_err, qemu_note};
use noct_physmem::{PAGE_PRESENT, PAGE_SIZE, PAGE_USER, PAGE_WRITEABLE};

#[inline(always)]
pub fn align_up(value: usize, align: usize) -> usize {
    debug_assert!(align.is_power_of_two(), "Alignment must be a power of two");
    (value + align - 1) & !(align - 1)
}

#[derive(Debug)]
pub enum LoadError {
    System(&'static str),
    ElfParser(ParseError),
}

#[derive(Debug)]
pub struct LoadInfo {
    physical_addr: usize,
    virtual_addr: usize,
    page_count: usize,
}

#[derive(Debug)]
pub struct ProgramHandle {
    path: String,

    entry_point: usize,
    loaded_segments: Vec<LoadInfo>,
}

impl ProgramHandle {
    pub fn run(&mut self, args: &[&str]) {
        let mut normalized: Vec<&str> = Vec::with_capacity(1 + args.len());
        normalized.push(&self.path);
        normalized.extend_from_slice(args);

        let ptrs = normalized.iter().map(|a| a.as_ptr()).collect::<Vec<_>>();

        let argc: u32 = ptrs.len() as u32;
        let argv = ptrs.as_ptr();

        unsafe {
            let entry: fn(u32, *const *const u8) = core::mem::transmute(self.entry_point);

            qemu_note!(
                "program < argc: {argc}; argv: {:x?}; entry: {:x?}",
                argv as usize,
                entry as usize
            );

            (entry)(argc, argv);
        }
    }
}

impl Drop for ProgramHandle {
    fn drop(&mut self) {
        for seg in &self.loaded_segments {
            unsafe {
                for page in 0..seg.page_count {
                    noct_physmem::unmap_single_page(
                        noct_physmem::get_kernel_page_directory(),
                        (seg.virtual_addr + (page * PAGE_SIZE as usize)) as u32,
                    )
                }

                noct_physmem::phys_free_multi_pages(seg.physical_addr as _, seg.page_count as _);
            }
        }

        qemu_note!("Should drop!");
    }
}

pub fn load_elf_file(path: &str) -> Result<ProgramHandle, LoadError> {
    let data = match noct_fs::read(path) {
        Ok(data) => data,
        Err(e) => {
            qemu_err!("Error: {e}");
            return Err(LoadError::System(e));
        }
    };

    let elf = match elf::ElfBytes::<AnyEndian>::minimal_parse(&data) {
        Ok(elf) => elf,
        Err(err) => {
            qemu_err!("parser error: {err}");
            return Err(LoadError::ElfParser(err));
        }
    };

    let segments = elf.segments().unwrap();

    let mut loaded_segments: Vec<LoadInfo> = Vec::new();

    for i in segments.iter() {
        // 1 = PH_LOAD
        if i.p_type != 1 {
            continue;
        }

        let ph_data = match elf.segment_data(&i) {
            Ok(data) => data,
            Err(e) => {
                qemu_err!("Error happened during load! ({e})");

                // Clean up
                for seg in loaded_segments {
                    unsafe {
                        for page in 0..seg.page_count {
                            noct_physmem::unmap_single_page(
                                noct_physmem::get_kernel_page_directory(),
                                (seg.virtual_addr + (page * PAGE_SIZE as usize)) as u32,
                            )
                        }

                        noct_physmem::phys_free_multi_pages(
                            seg.physical_addr as _,
                            seg.page_count as _,
                        );
                    }
                }

                // todo!("Clean up");

                return Err(LoadError::ElfParser(e));
            }
        };

        let page_count = core::cmp::max(
            align_up(i.p_memsz as usize, PAGE_SIZE as usize) / PAGE_SIZE as usize,
            1,
        );

        let physical_address = unsafe { noct_physmem::phys_alloc_multi_pages(page_count as _) };

        unsafe {
            noct_physmem::map_pages(
                noct_physmem::get_kernel_page_directory(),
                physical_address,
                i.p_vaddr as u32,
                page_count as u32 * PAGE_SIZE,
                PAGE_PRESENT | PAGE_USER | PAGE_WRITEABLE,
            );

            let memory_area = core::slice::from_raw_parts_mut(
                i.p_vaddr as *mut u8,
                (page_count as u32 * PAGE_SIZE) as usize,
            );

            memory_area.fill(0);

            memory_area[..ph_data.len()].copy_from_slice(ph_data);
        }

        loaded_segments.push(LoadInfo {
            physical_addr: physical_address as _,
            virtual_addr: i.p_vaddr as _,
            page_count,
        });
    }

    Ok(ProgramHandle {
        path: String::from(path),
        entry_point: elf.ehdr.e_entry as usize,
        loaded_segments,
    })
}
