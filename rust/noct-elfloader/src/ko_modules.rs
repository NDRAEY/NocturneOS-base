use core::alloc::Layout;

use alloc::{string::String, vec::Vec};
use elf::{
    abi::{ET_REL, R_X86_64_PC32, SHT_PROGBITS, SHT_REL, STB_GLOBAL, STT_FUNC, STT_NOTYPE, STT_SECTION}, endian::AnyEndian, ElfBytes
};
use noct_logger::{qemu_err, qemu_note, qemu_warn};

use crate::{LoadError, LoadInfo};

#[derive(Debug)]
pub struct ModuleHandle {
    path: String,

    entry_point: usize,
    loaded_segments: Vec<LoadInfo>,
}

pub struct MemoryPool {
    pointers: Vec<(*mut u8, Layout)>,
}

impl MemoryPool {
    pub fn new() -> Self {
        Self { pointers: Vec::new() }
    }

    pub fn allocate_array(&mut self, layout: Layout) -> *mut u8 {
        let ptr = unsafe { alloc::alloc::alloc(layout) };
        self.pointers.push((ptr, layout));
        ptr
    }
}

impl Drop for MemoryPool {
    fn drop(&mut self) {
        for (ptr, layout) in &self.pointers {
            unsafe { alloc::alloc::dealloc(*ptr, *layout) };
        }
    }
}

pub fn load_module(path: &str) -> Result<ModuleHandle, LoadError> {
    let data = match noct_fs::read(path) {
        Ok(data) => data,
        Err(e) => {
            qemu_err!("Error: {e}");
            return Err(LoadError::System(e));
        }
    };

    let elf = match ElfBytes::<AnyEndian>::minimal_parse(&data) {
        Ok(elf) => elf,
        Err(err) => {
            qemu_err!("parser error: {err}");
            return Err(LoadError::ElfParser(err));
        }
    };

    if elf.ehdr.e_type != ET_REL {
        qemu_err!("Not a relocateable file!");
        return Err(LoadError::InvalidELFType);
    }

    let (symtab, strtab) = elf.symbol_table().unwrap().unwrap();

    let (all_progbits, secnamtable) = elf
        .section_headers_with_strtab()
        .map(|x| {
            (
                x.0.unwrap().iter().filter(|x| x.sh_type == SHT_PROGBITS),
                x.1.unwrap(),
            )
        })
        .unwrap();

    let mut loaded_segments: Vec<(&str, usize)> = Vec::new();

    let mut mempool = MemoryPool::new();

    for i in all_progbits {
        let memsize = i.sh_size as usize;
        let ptr = mempool.allocate_array(Layout::array::<u8>(memsize).unwrap());

        let (data, _) = elf.section_data(&i).unwrap();

        unsafe { ptr.copy_from(data.as_ptr(), data.len()) };

        let name = secnamtable.get(i.sh_name as _).unwrap();

        loaded_segments.push((name, ptr.addr()));

        qemu_note!("{i:x?} {name} {ptr:x?}");
    }

    qemu_note!("{loaded_segments:x?}");

    let mut exploreable_symbols: Vec<(usize, &str, usize)> = Vec::new();

    for (idx, i) in symtab.iter().enumerate() {
        let mut name = strtab.get(i.st_name as _).unwrap();
        let total_offset: usize;

        if i.st_symtype() == STT_FUNC || i.st_symtype() == STT_SECTION {
            let base = match {
                let name_off = elf
                    .section_headers()
                    .unwrap()
                    .get(i.st_shndx as _)
                    .unwrap()
                    .sh_name;
                let name = secnamtable.get(name_off as _).unwrap();

                loaded_segments.iter().find(|x| x.0 == name)
            } {
                Some(x) => x,
                None => {
                    qemu_err!("Failed to look up section nr. {ndx}", ndx = i.st_shndx);
                    continue;
                }
            };

            if name.is_empty() {
                name = base.0;
            }

            total_offset = i.st_value as usize + base.1;
        } else if i.st_symtype() == STT_NOTYPE && i.st_bind() == STB_GLOBAL {
            qemu_note!("IMPLEMENT: {name}");

            if name == "_GLOBAL_OFFSET_TABLE_" {
                qemu_warn!("What the fuck is the `_GLOBAL_OFFSET_TABLE_`?");
                continue;
            }

            let offset = noct_ksymparser::resolve(name).unwrap();

            total_offset = offset;
        } else {
            continue;
        }

        qemu_note!("`{name}`: {total_offset:x}");
        exploreable_symbols.push((idx, name, total_offset));
    }

    qemu_note!("{exploreable_symbols:x?}");

    let rels = elf
        .section_headers_with_strtab()
        .map(|x| x.0.unwrap())
        .unwrap()
        .into_iter()
        .filter(|x| x.sh_type == SHT_REL);

    for rel in rels {
        let data = elf.section_data_as_rels(&rel).unwrap();

        // Assuming that every relocation section starts with ".rel"
        let target_relocation_section = &secnamtable.get(rel.sh_name as _).unwrap()[4..];

        if target_relocation_section == ".eh_frame" {
            qemu_warn!("Skipping .eh_frame");
            continue;
        }

        for i in data {
            let segment_offset = loaded_segments
                .iter()
                .find(|&x| x.0 == target_relocation_section)
                .unwrap()
                .1;

            let substitute = match exploreable_symbols
                .iter()
                .find(|x| x.0 == i.r_sym as usize)
                .map(|x| (x.1, x.2))
            {
                Some(x) => x,
                None => {
                    qemu_warn!("Not loading: {sym}", sym = i.r_sym);
                    continue;
                }
            }
            .1;

            let writing_address = (segment_offset + i.r_offset as usize) as *mut u32;
            let inner_value = unsafe { writing_address.read_unaligned() as usize };

            let value: usize;

            // qemu_note!("S: {substitute:x?} ( {substitute:x} - {segment_offset:x} ) A: {inner_value:x?} P: {:x?}", i.r_offset);

            if i.r_type == R_X86_64_PC32 {
                let s = substitute.overflowing_sub(segment_offset).0;
                let a = inner_value; // YAAAAAH THE FUCKING ADDEND IS A VALUE FFFFFFFC written in disasm!
                let p = i.r_offset as usize;

                value = (s.overflowing_add(a).0).overflowing_sub(p).0;
                unsafe { writing_address.write_unaligned(value as _) };
            } else if i.r_type == 1
            /* R_386_32 */
            {
                let s = substitute;
                let a = inner_value; // YAAAAAH THE FUCKING ADDEND IS A VALUE FFFFFFFC written in disasm!

                value = s + a;
                unsafe { writing_address.write_unaligned(value as u32) };
            } else {
                todo!("Other types of relocation: {}", i.r_type);
            }

            // qemu_note!(
            //     "{rt} Rel: {writing_address:x?}; => {value:x?}",
            //     rt = i.r_type
            // );
        }
    }

    let entry_point = exploreable_symbols
        .iter()
        .find(|&&x| x.1 == "module_init")
        .map(|x| x.2);

    if entry_point.is_none() {
        todo!("No entry point! Clean up and bail out!");
    }

    let entry_point = entry_point.unwrap();

    qemu_note!("Entry point at: {entry_point:x}");

    let ep: fn() -> () = unsafe { core::mem::transmute(entry_point) };

    ep();

    core::mem::forget(mempool);

    return Err(LoadError::System("not implemented yet"));
}
