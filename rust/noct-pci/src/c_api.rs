use crate::{PCI_DEVICES, find_device, find_device_by_class_and_subclass, pci_read32, pci_write};

#[unsafe(no_mangle)]
pub unsafe extern "C" fn pci_find_device(
    vendor: u16,
    device: u16,
    bus_ret: *mut u8,
    slot_ret: *mut u8,
    func_ret: *mut u8,
) -> bool {
    assert!(!bus_ret.is_null());
    assert!(!slot_ret.is_null());
    assert!(!func_ret.is_null());

    if let Some(dev) = find_device(vendor, device) {
        unsafe { *bus_ret = dev.bus };
        unsafe { *slot_ret = dev.slot };
        unsafe { *func_ret = dev.function };

        return true;
    }

    unsafe { *bus_ret = 0xff };
    unsafe { *slot_ret = 0xff };
    unsafe { *func_ret = 0xFF };

    false
}

#[unsafe(no_mangle)]
pub unsafe extern "C" fn pci_find_device_by_class_and_subclass(
    class: u8,
    subclass: u8,
    vendor_ret: *mut u16,
    deviceid_ret: *mut u16,
    bus_ret: *mut u8,
    slot_ret: *mut u8,
    func_ret: *mut u8,
) -> bool {
    assert!(!vendor_ret.is_null());
    assert!(!deviceid_ret.is_null());
    assert!(!bus_ret.is_null());
    assert!(!slot_ret.is_null());
    assert!(!func_ret.is_null());

    if let Some(dev) = find_device_by_class_and_subclass(class, subclass) {
        unsafe { *vendor_ret = dev.vendor };
        unsafe { *deviceid_ret = dev.device };
        unsafe { *bus_ret = dev.bus };
        unsafe { *slot_ret = dev.slot };
        unsafe { *func_ret = dev.function };

        return true;
    }

    unsafe { *bus_ret = 0xff };
    unsafe { *slot_ret = 0xff };
    unsafe { *func_ret = 0xff };

    false
}

#[unsafe(no_mangle)]
pub extern "C" fn pci_enable_bus_mastering(bus: u8, slot: u8, func: u8) {
    let mut command_register = pci_read32(bus, slot, func, 4);

    command_register |= 0x05;

    pci_write(bus, slot, func, 4, command_register);
}
