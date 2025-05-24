use core::arch::asm;

use x86::io::{inb, outb};

static PS2_DATA_PORT: u16 = 0x60;
static PS2_STATE_REG: u16 = 0x64;

#[unsafe(no_mangle)]
#[allow(non_upper_case_globals)]
static mut ps2_channel1_okay: bool = false;

#[unsafe(no_mangle)]
#[allow(non_upper_case_globals)]
static mut ps2_channel2_okay: bool = false;

/// Reads configuration byte register from PCI CONTROLLER
///
/// # Safety
/// It uses IO ports
#[unsafe(no_mangle)]
pub extern "C" fn ps2_read_configuration_byte() -> u8 {
    ps2_in_wait_until_empty();
    unsafe { outb(PS2_STATE_REG, 0x20) };

    ps2_read()
}

/// # Safety
/// It uses IO ports
#[unsafe(no_mangle)]
pub unsafe extern "C" fn ps2_write_configuration_byte(byte: u8) {
    ps2_in_wait_until_empty();
    unsafe { outb(PS2_STATE_REG, 0x60) };

    unsafe { ps2_write(byte) };
}

/// # Safety
/// It uses IO ports
#[unsafe(no_mangle)]
fn ps2_in_wait_until_empty() {
    while (unsafe { inb(PS2_STATE_REG) } & (1 << 1)) != 0 {
        unsafe {
            asm!("nop");
        }
    }
}

/// # Safety
/// It uses IO ports
#[unsafe(no_mangle)]
fn ps2_out_wait_until_full() {
    while (unsafe { inb(PS2_STATE_REG) } & 1) == 0 {}
}

/// Reads byte from PCI Controller
/// # Safety
/// It uses IO ports
#[unsafe(no_mangle)]
pub extern "C" fn ps2_read() -> u8 {
    ps2_out_wait_until_full();

    unsafe { inb(PS2_DATA_PORT) }
}

/// # Safety
/// It uses IO ports
#[unsafe(no_mangle)]
pub unsafe extern "C" fn ps2_write(byte: u8) {
    ps2_in_wait_until_empty();
    unsafe { outb(PS2_DATA_PORT, byte) };
}

pub fn ps2_disable_first_device() {
    unsafe {
        ps2_in_wait_until_empty();
        outb(PS2_STATE_REG, 0xAD); // 1
    };
}

pub fn ps2_disable_second_device() {
    ps2_in_wait_until_empty();
    unsafe { outb(PS2_STATE_REG, 0xA7) }; // 2
}

pub fn ps2_enable_first_device() {
    ps2_in_wait_until_empty();
    unsafe { outb(PS2_STATE_REG, 0xAE) }; // 1
}

pub fn ps2_enable_second_device() {
    ps2_in_wait_until_empty();
    unsafe { outb(PS2_STATE_REG, 0xA8) }; // 2
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

    ps2_out_wait_until_full();

    let reply: u8 = unsafe { inb(PS2_DATA_PORT) };

    if reply == 0x55 { Some(()) } else { None }
}

#[unsafe(no_mangle)]
pub extern "C" fn ps2_init() {
    ps2_disable_first_device();
    ps2_disable_second_device();

    ps2_flush();

    ps2_in_wait_until_empty();

    let conf: u8 = ps2_read_configuration_byte();
    unsafe { ps2_write_configuration_byte(conf & !(0b1000011)) };

    // TEST CONTROLLER

    let test_ok = ps2_test();

    if test_ok.is_none() {
        noct_logger::qemu_err!("PS/2 TEST ERROR!");
        return;
    }

    noct_logger::qemu_ok!("PS/2 test ok!");

    // Test first port

    ps2_in_wait_until_empty();
    unsafe { outb(PS2_STATE_REG, 0xAB) };

    ps2_out_wait_until_full();
    let mut result: u8 = unsafe { inb(PS2_DATA_PORT) };

    if result == 0x00 {
        noct_logger::qemu_ok!("Passed test for channel 1!");
        unsafe { ps2_channel1_okay = true };
    } else {
        noct_logger::qemu_err!("Channel 1: Test failed! Result: {:x}", result);
    }

    // Test second port

    unsafe {
        ps2_in_wait_until_empty();
        outb(PS2_STATE_REG, 0xA9);

        ps2_out_wait_until_full()
    };

    result = unsafe { inb(PS2_DATA_PORT) };

    if result == 0x00 {
        noct_logger::qemu_ok!("Passed test for channel 2!");
        unsafe { ps2_channel2_okay = true };
    } else {
        noct_logger::qemu_err!("Channel 2: Test failed! Result: {:x}", result);
    }

    ps2_enable_first_device();
    ps2_enable_second_device();
}
