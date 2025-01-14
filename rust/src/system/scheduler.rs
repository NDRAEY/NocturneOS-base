use super::regs::Registers;

extern "C" {
    fn task_switch_v2_wrapper(registers: Registers);
}

#[inline]
pub fn task_yield() {
    unsafe {
        task_switch_v2_wrapper(Registers::default());
    };
}
