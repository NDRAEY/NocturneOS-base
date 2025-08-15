use crate::{find_device, find_device_by_class_and_subclass, get_device, pci_read32, pci_write};

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

#[unsafe(no_mangle)]
pub unsafe extern "C" fn pci_get_bar(
    bus: u8,
    slot: u8,
    func: u8,
    bar: u8,
    type_out: *mut u8,
    address_out: *mut usize,
    length_out: *mut usize,
) {
    let dev = match get_device(bus, slot, func) {
        Some(dev) => dev,
        None => {
            return;
        }
    };

    if let Some(bar) = dev.read_bar(bar) {
        if !type_out.is_null() {
            unsafe { *type_out = bar.bar_type.into() };
        }
        if !address_out.is_null() {
            unsafe { *address_out = bar.address };
        }
        if !length_out.is_null() {
            unsafe { *length_out = bar.length };
        }
    }
}
