#![no_std]

use alloc::{
    string::{String, ToString},
    vec::Vec,
};

extern crate alloc;

const SMBIOS_32_ANCHOR: &[u8] = b"_SM_";

#[repr(C, packed)]
#[derive(Debug)]
pub struct SMBIOS32Header {
    pub anchor_string: [u8; 4],
    pub entry_point_checksum: u8,
    pub entry_point_length: u8,
    pub version_major: u8,
    pub version_minor: u8,
    pub structure_max_size: u16,
    pub entry_point_revision: u8,
    pub formatted_area: [u8; 5],
    pub intermediate_anchor_string: [u8; 5],
    pub intermediate_checksum: u8,
    pub structure_table_length: u16,
    pub structure_table_addr: u32,
    pub number_of_structs: u16,
    pub bcd_revision: u8,
}

// impl SMBIOS32 {
//     pub fn is_valid(&self) -> bool {
//         let len = self.entry_point_length;
//         let mut checksum: u8 = 0;

//         for i in 0..len {
//             // ...
//         }
//     }
// }

#[repr(C, packed)]
#[derive(Debug)]
pub struct SMBIOSEntryHeader {
    header_type: u8,
    length: u8,
    handle: u16,
}

#[derive(Debug)]
pub struct SMBIOS {
    address: usize,
    header: SMBIOS32Header,
}

#[derive(Debug)]
pub enum SMBIOSEntry {
    BIOS {
        vendor: String,
        firmware_version: String,
        segment_address: u16,
        release_date: String,
        rom_size: usize,
    },
    System {
        manufacturer: String,
        product_name: String,
        version: String,
        serial_number: String,
        uuid: [u8; 16],
        sku: String,
        family: String,
    },
}

impl SMBIOS {
    pub fn new_unchecked(addr: usize) -> Self {
        Self {
            address: addr,
            header: unsafe { (addr as *const SMBIOS32Header).read_volatile() },
        }
    }

    fn parse_string(&self, table_end: usize, nr: usize) -> Option<String> {
        if nr == 0 {
            return None;
        }

        let mut addr = table_end;
        let mut counter = 1;

        loop {
            let data = unsafe {
                core::slice::from_raw_parts(addr as *const u8, {
                    let mut a = addr as *const u8;
                    let mut len = 0usize;

                    while a.read() != 0 {
                        a = a.add(1);
                        len += 1;
                    }

                    len
                })
            };

            if data.len() == 0 {
                return None;
            }

            if counter == nr {
                return Some(String::from_utf8_lossy(data).to_string());
            }

            addr += data.len() + 1;

            counter += 1;
        }
    }

    pub fn scan(&self) -> Vec<SMBIOSEntry> {
        let mut entries = Vec::new();

        let mut addr = self.header.structure_table_addr;
        let len = self.header.structure_table_length;

        let mut found = 0;

        while addr < (addr + len as u32) {
            if found == self.header.number_of_structs {
                break;
            }

            let header = unsafe { (addr as *const SMBIOSEntryHeader).read() };

            if header.header_type == 127 {
                break;
            }

            if header.length == 0 {
                break;
            }

            let data = unsafe {
                core::slice::from_raw_parts(
                    addr as *const u8,
                    header.length as usize,
                )
            };

            let mut table_end =
                addr + header.length as u32;

            match header.header_type {
                0 => {
                    let ven = data[0x4];
                    let fw_ver = data[0x5];
                    let bios_start = ((data[0x7] as u16) << 8) | data[0x6] as u16;
                    let firmware_release_date = data[0x8];
                    let firmware_rom_size = data[0x9];

                    let venstr = self
                        .parse_string(table_end as _, ven as _)
                        .unwrap_or("Unknown".to_string());
                    let fw = self
                        .parse_string(table_end as _, fw_ver as _)
                        .unwrap_or("Unknown".to_string());
                    let fwrel = self
                        .parse_string(table_end as _, firmware_release_date as _)
                        .unwrap_or("Unknown".to_string());

                    entries.push(SMBIOSEntry::BIOS {
                        vendor: venstr,
                        firmware_version: fw,
                        segment_address: bios_start,
                        release_date: fwrel,
                        rom_size: firmware_rom_size as usize,
                    });
                }
                1 => {
                    let manufacturer = data[0x4];
                    let product_name = data[0x5];
                    let version = data[0x6];
                    let serial_number = data[0x7];
                    let uuid: &[u8] = &data[0x8..0x18];
                    let sku = data[0x19];
                    let family = data[0x1A];

                    let manufacturer = self
                        .parse_string(table_end as _, manufacturer as _)
                        .unwrap_or("Unknown".to_string());
                    let product_name = self
                        .parse_string(table_end as _, product_name as _)
                        .unwrap_or("Unknown".to_string());
                    let version = self
                        .parse_string(table_end as _, version as _)
                        .unwrap_or("Unknown".to_string());
                    let serial_number = self
                        .parse_string(table_end as _, serial_number as _)
                        .unwrap_or("Unknown".to_string());
                    let sku = self
                        .parse_string(table_end as _, sku as _)
                        .unwrap_or("Unknown".to_string());
                    let family = self
                        .parse_string(table_end as _, family as _)
                        .unwrap_or("Unknown".to_string());

                    entries.push(SMBIOSEntry::System {
                        manufacturer,
                        product_name,
                        version,
                        serial_number,
                        uuid: {
                            let mut n_uuid = [0u8; 16];
                            n_uuid.copy_from_slice(uuid);
                            n_uuid
                        },
                        sku,
                        family,
                    });
                }
                _ => (),
            }

            loop {
                let characters = unsafe { (table_end as *const [u8; 2]).read() };

                table_end += 1;

                if characters == [0, 0] {
                    table_end += 1;
                    break;
                }
            }

            addr = table_end;

            found += 1;
        }

        entries
    }
}

pub fn find_smbios() -> Option<SMBIOS> {
    let mut addr = 0x000F0000;

    while addr <= 0x00FFFFF {
        unsafe {
            let data = core::slice::from_raw_parts(addr as *const u8, 4);

            if data == SMBIOS_32_ANCHOR {
                return Some(SMBIOS::new_unchecked(addr));
            }
        }

        addr += 16;
    }

    None
}
