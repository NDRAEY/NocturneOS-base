#![allow(static_mut_refs)]

use core::cell::OnceCell;

use alloc::vec::Vec;
use noct_logger::qemu_log;
use spin::RwLock;
use x86::io::{inl, outl};

use noct_tty::{print, println};

use noct_timer::timestamp;

static PCI_ADDRESS_PORT: u16 = 0xCF8;
static PCI_DATA_PORT: u16 = 0xCFC;

static mut PCI_DEVICES: OnceCell<RwLock<Vec<PCIDevice>>> = OnceCell::new();

static PCI_DEVICE_TYPE_STRINGS: [(u8, u8, &str); 89] = [
    (0x00, 0x00, "Неизвестное устройство"),
    (0x00, 0x01, "VGA-совместимое устройство"),
    (0x01, 0x00, "Контроллер шины SCSI"),
    (0x01, 0x01, "IDE-контроллер"),
    (0x01, 0x02, "Контроллер гибких дисков"),
    (0x01, 0x03, "Контроллер шины IPI"),
    (0x01, 0x04, "RAID-контроллер"),
    (0x01, 0x05, "Контроллер АТА"),
    (0x01, 0x06, "SATA-контроллер"),
    (0x01, 0x07, "Последовательный подключенный контроллер SCSI"),
    (0x01, 0x80, "Другой контроллер запоминающих устройств"),
    (0x02, 0x00, "Ethernet-контроллер"),
    (0x02, 0x01, "Контроллер Token Ring"),
    (0x02, 0x02, "FDDI-контроллер"),
    (0x02, 0x03, "Контроллер банкомата"),
    (0x02, 0x04, "ISDN-контроллер"),
    (0x02, 0x05, "Контроллер WorldFip"),
    (0x02, 0x06, "PICMG 2.14 Мультивычисления"),
    (0x02, 0x80, "Другой сетевой контроллер"),
    (0x03, 0x00, "VGA-совместимый контроллер"),
    (0x03, 0x01, "XGA-контроллер"),
    (0x03, 0x02, "3D контроллер"),
    (0x03, 0x80, "Другой контроллер дисплея"),
    (0x04, 0x00, "Видео-устройство"),
    (0x04, 0x01, "Аудио-устройство"),
    (0x04, 0x02, "Компьютерное телефонное устройство"),
    (0x04, 0x03, "Аудио-устройство (4.3)"),
    (0x04, 0x80, "Другое мультимедийное устройство"),
    (0x05, 0x00, "Контроллер оперативной памяти"),
    (0x05, 0x01, "Флэш-контроллер"),
    (0x05, 0x80, "Другой контроллер памяти"),
    (0x06, 0x00, "Хост-мост"),
    (0x06, 0x01, "ISA мост"),
    (0x06, 0x02, "EISA мост"),
    (0x06, 0x03, "MCA мост"),
    (0x06, 0x04, "PCI-to-PCI мост"),
    (0x06, 0x05, "PCMCIA мост"),
    (0x06, 0x06, "NuBus мост"),
    (0x06, 0x07, "CardBus мост"),
    (0x06, 0x08, "RACEWay мост"),
    (0x06, 0x09, "PCI-to-PCI мост (Полупрозрачный)"),
    (0x06, 0x0A, "Хост-мост InfiniBand-PCI"),
    (0x06, 0x80, "Другое устройство моста"),
    (0x07, 0x00, "Последовательный контроллер"),
    (0x07, 0x01, "Параллельный порт"),
    (0x07, 0x02, "Многопортовый последовательный контроллер"),
    (0x07, 0x03, "Универсальный модем"),
    (0x07, 0x04, "IEEE 488.1/2 (GPIB) контроллер"),
    (0x07, 0x05, "Интеллектуальная карточка"),
    (0x07, 0x80, "Другое устройство связи"),
    (0x08, 0x00, "Программируемый контроллер прерываний"),
    (0x08, 0x01, "Контроллер прямого доступа к памяти"),
    (0x08, 0x02, "Системный таймер"),
    (0x08, 0x03, "Часы реального времени"),
    (0x08, 0x04, "Универсальный контроллер PCI (hot-plug)"),
    (0x08, 0x80, "Другая системная периферия"),
    (0x09, 0x00, "Контроллер клавиатуры"),
    (0x09, 0x01, "Цифровой преобразователь"),
    (0x09, 0x02, "Контроллер мыши"),
    (0x09, 0x03, "Контроллер сканера"),
    (0x09, 0x04, "Контроллер игрового порта"),
    (0x09, 0x80, "Другой контроллер ввода"),
    (0x0A, 0x00, "Универсальная док-станция"),
    (0x0A, 0x80, "Другая док-станция"),
    (0x0B, 0x00, "Процессор i386"),
    (0x0B, 0x01, "Процессор i486"),
    (0x0B, 0x02, "Процессор Pentium"),
    (0x0B, 0x10, "Процессор Alpha"),
    (0x0B, 0x20, "Процессор PowerPC"),
    (0x0B, 0x30, "Процессор MIPS"),
    (0x0B, 0x40, "Со-процессор"),
    (0x0C, 0x00, "Контроллер FireWire"),
    (0x0C, 0x01, "Контроллер ACCESS.bus"),
    (0x0C, 0x02, "SSA Контроллер"),
    (0x0C, 0x03, "USB Контроллер"),
    (0x0C, 0x04, "Волоконный канал"),
    (0x0C, 0x05, "SMBus"),
    (0x0C, 0x06, "InfiniBand"),
    (0x0C, 0x07, "Интерфейс IPMI SMIC"),
    (0x0C, 0x08, "Интерфейс SERCOS"),
    (0x0C, 0x09, "Интерфейс CANbus"),
    (0x0D, 0x00, "iRDA-совместимый контроллер"),
    (0x0D, 0x01, "Потребительский ИК-контроллер"),
    (0x0D, 0x10, "RF Контроллер"),
    (0x0D, 0x11, "Bluetooth Контроллер"),
    (0x0D, 0x12, "Broadband Контроллер"),
    (0x0D, 0x20, "802.11a (Wi-Fi) Ethernet-контроллер"),
    (0x0D, 0x21, "802.11b (Wi-Fi) Ethernet-контроллер"),
    (0x0D, 0x80, "Другой беспроводной контроллер"),
];

#[derive(Debug, Default)]
pub struct PCIDevice {
    pub vendor: u16,
    pub device: u16,

    pub bus: u8,
    pub slot: u8,
    pub function: u8,

    pub class: u8,
    pub subclass: u8,

    pub header_type: u8,
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

        inl(PCI_DATA_PORT)
    }
}

#[no_mangle]
pub extern "C" fn pci_read16(bus: u8, slot: u8, function: u8, offset: u8) -> u16 {
    let addr: u32 = ((bus as u32) << 16)
        | ((slot as u32) << 11)
        | ((function as u32) << 8)
        | ((offset & 0xFC) as u32)
        | 0x80000000; //yes, this line is copied from osdev

    unsafe {
        outl(PCI_ADDRESS_PORT, addr);

        (inl(PCI_DATA_PORT) >> ((offset & 2) * 8)) as u16
    }
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
        vendor,
        device,
        ..PCIDevice::default()
    };

    unsafe {
        let result: u8 = pci_find_device(
            vendor,
            device,
            &mut dev.bus,
            &mut dev.slot,
            &mut dev.function,
        );

        if result == 0 {
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
    pci_read16(bus, slot, function, 0)
}

#[no_mangle]
pub fn pci_scan_everything() {
    unsafe {
        let devices = PCI_DEVICES.get_or_init(|| RwLock::new(Vec::new()));

        let mut guard = devices.write();
        guard.clear();
    }

    let start_time = timestamp();

    for bus in 0..=255 {
        for slot in 0..=31 {
            let mut hdrtype: u8 = 0;

            let mut vendor: u16 = pci_get_vendor(bus, slot, 0);

            if vendor != 0xFFFF {
                let clid: u8 = ((pci_read16(bus, slot, 0, 0xA) >> 8) & 0xff) as u8;
                let sclid: u8 = ((pci_read16(bus, slot, 0, 0xA)) & 0xff) as u8;
                let device: u16 = (pci_read16(bus, slot, 0, 0x2) & 0xffff) as u16;

                hdrtype = (pci_read16(bus, slot, 0, 0xE) & 0xff) as u8;

                unsafe {
                    let dev = PCIDevice {
                        class: clid,
                        subclass: sclid,
                        bus,
                        slot,
                        function: 0,
                        header_type: hdrtype,
                        vendor,
                        device,
                    };

                    // qemu_log!("{:?}", &dev);

                    PCI_DEVICES.get_mut().unwrap().write().push(dev);
                }
            }

            if (hdrtype & 0x80) != 0 {
                for func in 1..8 {
                    vendor = pci_get_vendor(bus, slot, func);

                    if vendor != 0xFFFF {
                        let clid = ((pci_read16(bus, slot, func, 0xA) >> 8) & 0xff) as u8;
                        let sclid = (pci_read16(bus, slot, func, 0xA) & 0xff) as u8;
                        let device = (pci_read16(bus, slot, func, 0x2) & 0xffff) as u16;

                        unsafe {
                            let dev = PCIDevice {
                                class: clid,
                                subclass: sclid,
                                bus,
                                slot,
                                function: func,
                                header_type: hdrtype,
                                vendor,
                                device,
                            };

                            // qemu_log!("{:?}", &dev);

                            PCI_DEVICES.get_mut().unwrap().write().push(dev);
                        }
                    }
                }
            }
        }
    }

    let elapsed = timestamp() - start_time;

    noct_il::log!("PCI: scan end in {} ms", elapsed);

    unsafe {
        noct_il::log!("PCI: Found {} devices", PCI_DEVICES.get().unwrap().read().len());
    }
}

#[no_mangle]
pub unsafe fn pci_find_device(
    vendor: u16,
    device: u16,
    bus_ret: *mut u8,
    slot_ret: *mut u8,
    func_ret: *mut u8,
) -> u8 {
    for dev in PCI_DEVICES.get().unwrap().read().iter() {
        if dev.vendor == vendor && dev.device == device {
            *bus_ret = dev.bus;
            *slot_ret = dev.slot;
            *func_ret = dev.function;

            return 1;
        }
    }

    *bus_ret = 0xff;
    *slot_ret = 0xff;
    *func_ret = 0xFF;

    0
}

#[no_mangle]
pub unsafe fn pci_find_device_by_class_and_subclass(
    class: u8,
    subclass: u8,
    vendor_ret: *mut u16,
    deviceid_ret: *mut u16,
    bus_ret: *mut u8,
    slot_ret: *mut u8,
    func_ret: *mut u8,
) -> u8 {
    for dev in PCI_DEVICES.get().unwrap().read().iter() {
        if dev.class == class && dev.subclass == subclass {
            *vendor_ret = dev.vendor;
            *deviceid_ret = dev.device;
            *bus_ret = dev.bus;
            *slot_ret = dev.slot;
            *func_ret = dev.function;

            return 1;
        }
    }

    *bus_ret = 0xff;
    *slot_ret = 0xff;
    *func_ret = 0xff;

    0
}

#[no_mangle]
pub fn pci_print_nth(
    class: u8,
    subclass: u8,
    bus: u8,
    slot: u8,
    hdr: u8,
    vendor: u16,
    device: u16,
    func: u8,
) {
    print!(
        "- {}:{}:{}:{}.{}: {:x}:{:x} -> {}",
        class,
        subclass,
        bus,
        slot,
        func,
        vendor,
        device,
        PCI_DEVICE_TYPE_STRINGS
            .iter()
            .find(|x| x.0 == class && x.1 == subclass)
            .unwrap_or(&(0, 0, "Unknown"))
            .2
    );

    if (hdr & 0x80) == 0 {
        print!(" [Multifunc]");
    }

    let bar0 = pci_read32(bus, slot, func, 0x10);
    let bar1 = pci_read32(bus, slot, func, 0x10 + 4);
    let bar2 = pci_read32(bus, slot, func, 0x10 + (2 * 4));
    let bar3 = pci_read32(bus, slot, func, 0x10 + (3 * 4));
    let bar4 = pci_read32(bus, slot, func, 0x10 + (4 * 4));
    let bar5 = pci_read32(bus, slot, func, 0x10 + (5 * 4));

    print!(
        "\n  Addresses: [{:08x}, {:08x}, {:08x}, {:08x}, {:08x}, {:08x}]",
        bar0, bar1, bar2, bar3, bar4, bar5
    );

    println!();
}

#[no_mangle]
pub fn pci_print_list() {
    let devs = unsafe { PCI_DEVICES.get().unwrap().read() };

    for dev in devs.iter() {
        pci_print_nth(
            dev.class,
            dev.subclass,
            dev.bus,
            dev.slot,
            dev.header_type,
            dev.vendor,
            dev.device,
            dev.function,
        );
    }
}
