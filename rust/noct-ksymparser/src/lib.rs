#![no_std]

unsafe extern "C" {
	static NOCTURNE_ksym_data_start: usize;
	static NOCTURNE_ksym_data_end: usize;
}

#[inline]
pub fn symbols() -> &'static [u8] {
	let size = NOCTURNE_ksym_data_end - NOCTURNE_ksym_data_start;

	core::slice::from_raw_pointer(
		NOCTURNE_ksym_data_start as *const u8,
		size
	)
}

pub fn resolve(name: &str) -> Option<usize> {
	todo!()
}
