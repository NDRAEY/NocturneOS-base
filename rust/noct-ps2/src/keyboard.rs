use noct_input::keyboard_buffer_put;
use noct_interrupts::{IRQ1, register_interrupt_handler, registers_t};
use noct_logger::{qemu_err, qemu_ok};
use x86::io::inb;

use crate::{
    PS2_STATE_REG,
    controller::{ps2_read, ps2_read_configuration_byte, ps2_write, ps2_write_configuration_byte},
};

#[unsafe(no_mangle)]
pub extern "C" fn ps2_keyboard_init() {
    let mut stat: u8;

    unsafe { ps2_write(0xf4) };
    stat = ps2_read();

    if stat != 0xfa {
        qemu_err!("Keyboard error: Enable fail");
        return;
    }

    unsafe { ps2_write(0xf0) };
    stat = ps2_read();

    if stat != 0xfa {
        qemu_err!("Keyboard error: Scancode set fail");
        return;
    }

    unsafe { ps2_write(0) };
    stat = ps2_read();

    if stat != 0xfa {
        qemu_err!("Keyboard error: Zero fail");
        return;
    }

    let _scancode = ps2_read() & 0b11;

    unsafe { ps2_write(0xf3) };
    stat = ps2_read();

    if stat != 0xfa {
        qemu_err!("Keyboard error: Repeat fail");
        return;
    }

    unsafe { ps2_write(0) };
    stat = ps2_read();

    if stat != 0xfa {
        qemu_err!("Keyboard error: Zero fail (phase 2)");
        return;
    }

    let conf = ps2_read_configuration_byte();

    unsafe { ps2_write_configuration_byte(conf | 0b1000001) };
}

#[unsafe(no_mangle)]
pub extern "C" fn keyboard_handler(_regs: registers_t) {
    let kbdstatus = unsafe { inb(PS2_STATE_REG) };

    if (kbdstatus & 0x01) != 0 {
        keyboard_buffer_put(ps2_read() as u32);
    }
}

#[unsafe(no_mangle)]
pub fn ps2_keyboard_install_irq() {
    unsafe { register_interrupt_handler(IRQ1 as _, Some(keyboard_handler)) };
    qemu_ok!("PS/2 keyboard interrupt installed!");
}
