use core::arch::asm;

use x86::io::{inb, outb};

use crate::{qemu_err, qemu_ok};

static PS2_DATA_PORT: u16 = 0x60;
static PS2_STATE_REG: u16 = 0x64;

#[no_mangle]
#[allow(non_upper_case_globals)]
static mut ps2_channel1_okay: bool = false;

#[no_mangle]
#[allow(non_upper_case_globals)]
static mut ps2_channel2_okay: bool = false;

/// Reads configuration byte register from PCI CONTROLLER
///
/// # Safety
/// It uses IO ports
#[no_mangle]
pub unsafe extern "C" fn ps2_read_configuration_byte() -> u8 {
    ps2_in_wait_until_empty();
    outb(PS2_STATE_REG, 0x20);

    ps2_read()
}

/// # Safety
/// It uses IO ports
#[no_mangle]
pub unsafe extern "C" fn ps2_write_configuration_byte(byte: u8) {
    ps2_in_wait_until_empty();
    outb(PS2_STATE_REG, 0x60);

    ps2_write(byte);
}

/// # Safety
/// It uses IO ports
#[no_mangle]
unsafe fn ps2_in_wait_until_empty() {
    while (inb(PS2_STATE_REG) & (1 << 1)) != 0 {
        asm!("nop");
    }
}

/// # Safety
/// It uses IO ports
#[no_mangle]
unsafe fn ps2_out_wait_until_full() {
    while (inb(PS2_STATE_REG) & 1) == 0 {}
}

/// Reads byte from PCI Controller
/// # Safety
/// It uses IO ports
#[no_mangle]
pub unsafe extern "C" fn ps2_read() -> u8 {
    ps2_out_wait_until_full();

    inb(PS2_DATA_PORT)
}

/// # Safety
/// It uses IO ports
#[no_mangle]
pub unsafe extern "C" fn ps2_write(byte: u8) {
    ps2_in_wait_until_empty();
    outb(PS2_DATA_PORT, byte);
}

/// # Safety
/// It uses IO ports
pub fn ps2_disable_first_device() {
    unsafe {
        ps2_in_wait_until_empty();
        outb(PS2_STATE_REG, 0xAD); // 1
    };
}

/// # Safety
/// It uses IO ports
pub unsafe fn ps2_disable_second_device() {
    ps2_in_wait_until_empty();
    outb(PS2_STATE_REG, 0xA7); // 2
}

/// # Safety
/// It uses IO ports
pub unsafe fn ps2_enable_first_device() {
    ps2_in_wait_until_empty();
    outb(PS2_STATE_REG, 0xAE); // 1
}

/// # Safety
/// It uses IO ports
pub unsafe fn ps2_enable_second_device() {
    ps2_in_wait_until_empty();
    outb(PS2_STATE_REG, 0xA8); // 2
}

pub fn ps2_flush() {
    unsafe {
        while inb(PS2_STATE_REG) & 1 != 0 {
            inb(PS2_DATA_PORT);
        }
    }
}

fn ps2_test() -> Option<()> {
    unsafe {
        ps2_in_wait_until_empty();

        outb(PS2_STATE_REG, 0xAA);
    } // Test command

    unsafe { ps2_out_wait_until_full() };

    let reply: u8 = unsafe { inb(PS2_DATA_PORT) };

    if reply == 0x55 {
        Some(())
    } else {
        None
    }
}

#[no_mangle]
pub extern "C" fn ps2_init() {
    unsafe {
        ps2_disable_first_device();
        ps2_disable_second_device();

        ps2_flush();

        ps2_in_wait_until_empty();
    };

    let conf: u8 = unsafe { ps2_read_configuration_byte() };
    unsafe { ps2_write_configuration_byte(conf & !(0b1000011)) };

    // TEST CONTROLLER

    let test_ok = ps2_test();

    if test_ok.is_none() {
        qemu_err!("PS/2 TEST ERROR!");
        return;
    }

    qemu_ok!("PS/2 test ok!");

    // Test first port

    unsafe { ps2_in_wait_until_empty() };
    unsafe { outb(PS2_STATE_REG, 0xAB) };

    unsafe { ps2_out_wait_until_full() };
    let mut result: u8 = unsafe { inb(PS2_DATA_PORT) };

    if result == 0x00 {
        qemu_ok!("Passed test for channel 1!");
        unsafe { ps2_channel1_okay = true };
    } else {
        qemu_err!("Channel 1: Test failed! Result: {:x}", result);
    }

    // Test second port

    unsafe {
        ps2_in_wait_until_empty();
        outb(PS2_STATE_REG, 0xA9);

        ps2_out_wait_until_full()
    };
    result = unsafe { inb(PS2_DATA_PORT) };

    if result == 0x00 {
        qemu_ok!("Passed test for channel 2!");
        unsafe { ps2_channel2_okay = true };
    } else {
        qemu_err!("Channel 2: Test failed! Result: {:x}", result);
    }

    unsafe {
        ps2_enable_first_device();
        ps2_enable_second_device();
    }
}
