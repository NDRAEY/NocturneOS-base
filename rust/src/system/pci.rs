use core::cell::OnceCell;

use alloc::vec::Vec;
use x86::io::{inl, outl};

use crate::{print, println, qemu_log};

use super::timer::timestamp;

static PCI_ADDRESS_PORT: u16 = 0xCF8;
static PCI_DATA_PORT: u16 = 0xCFC;

static mut PCI_DEVICES: OnceCell<Vec<PCIDevice>> = OnceCell::new();

extern "C" {
    fn pci_get_device(bus: u8, slot: u8, function: u8) -> u16;
}

#[derive(Debug, Default)]
pub struct PCIDevice {
    pub vendor: u16,
    pub device: u16,
    
    pub bus: u8,
    pub slot: u8,
    pub function: u8,

    pub class: u8,
    pub subclass: u8,
    
    pub header_type: u8
}

#[no_mangle]
pub extern "C" fn pci_read32(bus: u8, slot: u8, function: u8, offset: u8) -> u32 {
    let addr: u32 = ((bus as u32) << 16)
        | ((slot as u32) << 11)
        | ((function as u32) << 8)
        | (offset as u32)
        | 0x80000000; //yes, this line is copied from osdev

    unsafe {
        outl(PCI_ADDRESS_PORT, addr);
    }
    unsafe { inl(PCI_DATA_PORT) }
}

#[no_mangle]
pub extern "C" fn pci_write(bus: u8, slot: u8, func: u8, offset: u8, value: u32) {
    let addr: u32 = ((bus as u32) << 16)
        | ((slot as u32) << 11)
        | ((func as u32) << 8)
        | ((offset as u32) & 0xfc)
        | 0x80000000;
    unsafe {
        outl(PCI_ADDRESS_PORT, addr);
        outl(PCI_DATA_PORT, value);
    }
}

impl PCIDevice {
    /**
     * @brief Чтение данных из шины PCI
     * @param bus Шина
     * @param slot Слот
     * @param function Функция
     * @param offset Отступ
     * @return Значение поля
     */
    pub fn read(&self, offset: u8) -> u32 {
        pci_read32(self.bus, self.slot, self.function, offset)
    }

    pub fn read_bar(&self, bar: u8) -> u32 {
        self.read(0x10 + (bar * 4))
    }

    pub fn write(&self, offset: u8, value: u32) {
        pci_write(self.bus, self.slot, self.function, offset, value);
    }
}

#[no_mangle]
pub fn pci_enable_bus_mastering(bus: u8, slot: u8, func: u8) {
    let mut command_register = pci_read32(bus, slot, func, 4);
    
    command_register |= 0x05;
    
    pci_write(bus, slot, func, 4, command_register);
}

pub fn find_device(vendor: u16, device: u16) -> Option<PCIDevice> {
    let mut dev: PCIDevice = PCIDevice {
        vendor, device,
        ..PCIDevice::default()
    };

    unsafe {
        pci_find_device(
            vendor.into(),
            device.into(),
            &mut dev.bus,
            &mut dev.slot,
            &mut dev.function,
        );

        let devid: u16 = pci_get_device(dev.bus, dev.slot, dev.function);

        if devid == 0xffff {
            return None;
        }
    }

    Some(dev)
}

pub fn find_device_by_class_and_subclass(class: u8, subclass: u8) -> Option<PCIDevice> {
    let mut dev: PCIDevice = PCIDevice::default();

    unsafe {
        pci_find_device_by_class_and_subclass(
            class,
            subclass,
            &mut dev.vendor,
            &mut dev.device,
            &mut dev.bus,
            &mut dev.slot,
            &mut dev.function,
        );

        if dev.vendor == 0 || dev.device == 0 {
            return None;
        }
    }

    Some(dev)
}

#[inline(always)]
pub fn pci_get_vendor(bus: u8, slot: u8, function: u8) -> u16 {
    return (pci_read32(bus, slot, function, 0) & 0xffff) as u16;
}

#[no_mangle]
pub fn pci_scan_everything() {
    unsafe {
        if PCI_DEVICES.get().is_none() {
            PCI_DEVICES.set(Vec::new());
        } else {
            PCI_DEVICES.get_mut().unwrap().clear();
        }
    }

    let start_time = timestamp();

    for bus in 0..255 {
        for slot in 0..31 {
            let mut hdrtype: u8 = 0;
            let mut clid: u8 = 0;
            let mut sclid: u8 = 0;
            let mut device: u16 = 0;

            let mut vendor: u16 = pci_get_vendor(bus, slot, 0);

            if vendor != 0xFFFF {
                clid = (pci_read32(bus, slot, 0, 0xB) & 0xff) as u8;
                sclid = (pci_read32(bus, slot, 0, 0xA) & 0xff) as u8;
                hdrtype = (pci_read32(bus, slot, 0, 0xE) & 0xff) as u8;
                device = (pci_read32(bus, slot, 0, 0x2) & 0xffff) as u16;

                unsafe {
                    let dev = PCIDevice {
                        class: clid,
                        subclass: sclid,
                        bus: bus,
                        slot: slot,
                        function: 0,
                        header_type: hdrtype,
                        vendor: vendor,
                        device: device
                    };

                    qemu_log!("{:?}", &dev);

                    PCI_DEVICES.get_mut().unwrap().push(dev);
                }
            }

            if (hdrtype & 0x80) == 1 {
                for func in 1..8 {
                    vendor = pci_get_vendor(bus, slot, func);

                    if vendor != 0xFFFF {
                        clid = (pci_read32(bus, slot, func, 0xB) & 0xff) as u8;
                        sclid = (pci_read32(bus, slot, func, 0xA) & 0xff) as u8;
                        device = (pci_read32(bus, slot, func, 0x2) & 0xffff) as u16;
                        
                        unsafe {
                            let dev = PCIDevice {
                                class: clid,
                                subclass: sclid,
                                bus: bus,
                                slot: slot,
                                function: func,
                                header_type: hdrtype,
                                vendor: vendor,
                                device: device
                            };

                            qemu_log!("{:?}", &dev);

                            PCI_DEVICES.get_mut().unwrap().push(dev);
                        }
                    }
                }
            }
        }
    }

    let elapsed = timestamp() - start_time;
    
    qemu_log!("PCI scan end in {} ms", elapsed);
    
    unsafe {
        qemu_log!("Found {} devices", PCI_DEVICES.get().unwrap().len());
    }
}

#[no_mangle]
pub unsafe fn pci_find_device(vendor: u16, device: u16, bus_ret: *mut u8, slot_ret: *mut u8, func_ret: *mut u8) {
    for dev in PCI_DEVICES.get().unwrap() {
        if dev.vendor == vendor && dev.device == device {
            *bus_ret = dev.bus;
            *slot_ret = dev.slot;
            *func_ret = dev.function;

            return;
        }
    }

	*bus_ret = 0xff;
    *slot_ret = 0xff;
    *func_ret = 0xFF;
}

#[no_mangle]
pub unsafe fn pci_find_device_by_class_and_subclass(class: u8, subclass: u8,
    vendor_ret: *mut u16, deviceid_ret: *mut u16,
    bus_ret: *mut u8, slot_ret: *mut u8, func_ret: *mut u8) {

    for dev in PCI_DEVICES.get().unwrap() {
        if dev.class == class && dev.subclass == subclass {
            *vendor_ret = dev.vendor;
            *deviceid_ret = dev.device;
            *bus_ret = dev.bus;
            *slot_ret = dev.slot;
            *func_ret = dev.function;
        }
            
        return;
    }

    *bus_ret = 0xff;
    *slot_ret = 0xff;
    *func_ret = 0xff;
}

#[no_mangle]
pub unsafe fn pci_print_nth(class: u8, subclass: u8, bus: u8, slot: u8, hdr: u8, vendor: u16, device: u16, func: u8) {
    print!("{}:{}:{}:{}.{}: {:x} {:x} ",
                class,
                subclass,
                bus,
                slot,
                func,
                vendor,
                device);

    if (hdr & 0x80) == 0 {
        print!("[Multifunc]");
    }

    let bar0 = pci_read32(bus, slot, func, 0x10 + (0 * 4));
    let bar1 = pci_read32(bus, slot, func, 0x10 + (1 * 4));
    let bar2 = pci_read32(bus, slot, func, 0x10 + (2 * 4));
    let bar3 = pci_read32(bus, slot, func, 0x10 + (3 * 4));
    let bar4 = pci_read32(bus, slot, func, 0x10 + (4 * 4));
    let bar5 = pci_read32(bus, slot, func, 0x10 + (5 * 4));

    print!("\nAddresses: [{:x}, {:x}, {:x}, {:x}, {:x}, {:x}]",
                bar0,
                bar1,
                bar2,
                bar3,
                bar4,
                bar5);

    println!("");
}

#[no_mangle]
pub unsafe fn pci_print_list() {
    for dev in PCI_DEVICES.get().unwrap() {
        pci_print_nth(
                dev.class,
                dev.subclass,
                dev.bus,
                dev.slot,
                dev.header_type,
                dev.vendor,
                dev.device,
                dev.function
        );
    }
}
