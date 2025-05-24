#![no_std]

unsafe extern "C" {
    static NOCTURNE_ksym_data_start: usize;
    static NOCTURNE_ksym_data_end: usize;
}

#[inline]
pub const fn symbols() -> Option<&'static [u8]> {
    unsafe {
        if NOCTURNE_ksym_data_start == 0 || NOCTURNE_ksym_data_end == 0 {
            return None;
        }

        let size = NOCTURNE_ksym_data_end - NOCTURNE_ksym_data_start;

        Some(core::slice::from_raw_parts(
            NOCTURNE_ksym_data_start as *const u8,
            size,
        ))
    }
}

pub fn resolve(name: &str) -> Option<usize> {
    let symbols = symbols()?;
    let symlen = symbols.len();

    let mut index = 0;
    while index < symlen {
        let address = u32::from_le_bytes(symbols[index..index + 4].try_into().unwrap());
        let namelen = symbols[index + 4] as usize;
        let sym_name = &symbols[index + 5..][..namelen];

        if sym_name == name.as_bytes() {
            return Some(address as usize);
        }

        index += 4 + 1 + namelen;
    }

    None
}
