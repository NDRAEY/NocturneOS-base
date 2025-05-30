use core::alloc::Layout;

use alloc::{string::String, vec::Vec};
use elf::{ElfBytes, abi::SHT_PROGBITS, endian::AnyEndian};
use noct_logger::{qemu_err, qemu_note};

use crate::{LoadError, LoadInfo};

#[derive(Debug)]
pub struct ModuleHandle {
    path: String,

    entry_point: usize,
    loaded_segments: Vec<LoadInfo>,
}

pub fn load_module(path: &str) -> Result<ModuleHandle, LoadError> {
    let mut data = match noct_fs::read(path) {
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

    let (symtab, strtab) = elf.symbol_table().unwrap().unwrap();

    let imports: Vec<_> = symtab
        .iter()
        .filter(|x| x.st_symtype() == 0 && x.st_name != 0)
        // .map(|a| strtab.get(a.st_name as _))
        .collect();

    let exports: Vec<_> = symtab
        .iter()
        .filter(|x| x.st_symtype() == 2 && x.st_size > 0)
        // .map(|a| strtab.get(a.st_name as _))
        .collect();

    qemu_note!("{imports:#?}");
    qemu_note!("{exports:#?}");

    let rel = elf.section_header_by_name(".rel.text").unwrap().unwrap();
    let data = elf.section_data_as_rels(&rel).unwrap();

    for i in data {
        qemu_note!("{i:?}");
    }

    let all_progbits = elf
        .section_headers()
        .unwrap()
        .iter()
        .filter(|x| x.sh_type == SHT_PROGBITS);
    let mut loaded_segments: Vec<(&str, usize)> = Vec::new();

    for i in all_progbits {
        let memsize = i.sh_size as usize;
        let ptr = unsafe { alloc::alloc::alloc(Layout::array::<u8>(memsize).unwrap()) };

        let (data, _) = elf.section_data(&i).unwrap();

        unsafe { ptr.copy_from(data.as_ptr(), data.len()) };

        let name = strtab.get(i.sh_name as _).unwrap();

        loaded_segments.push((name, ptr.addr()));

        qemu_note!("{i:x?} {name} {ptr:?}");
    }

    qemu_note!("{loaded_segments:?}");

    return Err(LoadError::System("not implemented yet"));
}
