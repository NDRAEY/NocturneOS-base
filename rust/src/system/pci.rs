use x86::io::{inl, outl};

static PCI_ADDRESS_PORT: u16 = 0xCF8;
static PCI_DATA_PORT: u16 = 0xCFC;

extern "C" {
    // fn pci_read_confspc_word(bus: u8, slot: u8, func: u8, offset: u8) -> u16;
    fn pci_find_device(
        vendor: u16,
        device: u16,
        bus_ret: *mut u8,
        slot_ret: *mut u8,
        function_ret: *mut u8,
    );
    fn pci_find_device_by_class_and_subclass(
        class: u16,
        subclass: u16,
        vendor_ret: *mut u16,
        devid_ret: *mut u16,
        bus_ret: *mut u8,
        slot_ret: *mut u8,
        function: *mut u8,
    );
    fn pci_get_device(bus: u8, slot: u8, function: u8) -> u16;
}

pub struct PCIDevice {
    pub vendor: u16,
    pub device_id: u16,
    pub bus: u8,
    pub slot: u8,
    pub function: u8,
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

    pub fn enable_bus_mastering(&self) {
        let mut command_register = self.read(4);
        command_register |= 0x05;
        self.write(4, command_register);
    }
}

pub fn find_device(vendor: u16, device: u16) -> Option<PCIDevice> {
    let mut dev: PCIDevice = PCIDevice {
        vendor,
        device_id: device,
        bus: 0,
        slot: 0,
        function: 0,
    };

    unsafe {
        pci_find_device(
            vendor,
            device,
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

pub fn find_device_by_class_and_subclass(class: u16, subclass: u16) -> Option<PCIDevice> {
    let mut dev: PCIDevice = PCIDevice {
        vendor: 0,
        device_id: 0,
        bus: 0,
        slot: 0,
        function: 0,
    };

    unsafe {
        pci_find_device_by_class_and_subclass(
            class,
            subclass,
            &mut dev.vendor,
            &mut dev.device_id,
            &mut dev.bus,
            &mut dev.slot,
            &mut dev.function,
        );

        if dev.vendor == 0 || dev.device_id == 0 {
            return None;
        }
    }

    Some(dev)
}
