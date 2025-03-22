use crate::qemu_log;
use raw_cpuid::*;

/// Gets a CPU brand (model name)
/// If output is null, only length will be updated
#[no_mangle]
pub unsafe extern "C" fn get_cpu_brand(output: *mut core::ffi::c_char, length: *mut usize) {
    let id = CpuId::default();
    let binding = id.get_processor_brand_string().unwrap();
    let brand = binding.as_str();
    let len = brand.len();

    if !length.is_null() {
        *length = len;
    }

    if !output.is_null() {
        for (nr, i) in brand.bytes().enumerate() {
            output.add(nr).write(i as i8);
        }
    }

    qemu_log!("Brand is: {}", brand);
}