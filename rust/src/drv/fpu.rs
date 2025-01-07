use core::arch::*;

static mut FPU_INITIALIZED: bool = false;

/// Возвращает статус FPU
#[no_mangle]
pub extern "C" fn fpu_is_initialized() -> bool {
    return unsafe { FPU_INITIALIZED };
}

/// Для инициализации FPU
unsafe fn fpu_fldcw(cw: u16) {
    asm!("fldcw [{}]", in(reg) &cw);
}

/// Инициализация FPU
#[no_mangle]
pub extern "C" fn fpu_init() {
    let mut cr4: u32;

    unsafe {
        asm!("mov {0:e}, cr4", out(reg) cr4);
    }

    cr4 |= 0x200;

    unsafe {
        asm!("mov cr4, {0:e}", in(reg) cr4);

        fpu_fldcw(0x37F);
        FPU_INITIALIZED = true;
    }
}
