use alloc::string::String;
use raw_cpuid::CpuId;

use crate::{
    println,
    system::{
        self, mem, version::{architecture, version_name}
    },
};

use super::ShellContext;

pub static SYSINFO_COMMAND_ENTRY: crate::shell::ShellCommandEntry =
    ("sysinfo", sysinfo, Some("System info"));

pub fn sysinfo(_context: &mut ShellContext, _args: &[&str]) -> Result<(), usize> {
    let (maj, min, patch) = crate::system::version::version();
    let arch = architecture();
    let vername = version_name();

    println!("NocturneOS {maj}.{min}.{patch} \"{vername}\" for {arch}");

    #[cfg(target_arch = "x86")]
    {
        let smbios = noct_smbios::find_smbios().unwrap();

        for i in smbios.scan() {
            match i {
                noct_smbios::SMBIOSEntry::BIOS {
                    vendor,
                    firmware_version,
                    segment_address,
                    release_date,
                    rom_size,
                } => {
                    println!("BIOS:");
                    println!("\tVendor: {vendor}");
                    println!("\tFirmware: {firmware_version}");
                    println!("\tRelease date: {release_date}");
                    println!("\tSegment Address: {segment_address:x}");
                    println!("\tROM size: {rom_size}");
                }
                noct_smbios::SMBIOSEntry::System {
                    manufacturer,
                    product_name,
                    version,
                    serial_number,
                    uuid,
                    sku,
                    family,
                } => {
                    println!("System:");
                    println!("\tManufacturer: {manufacturer}");
                    println!("\tProduct name: {product_name}");
                    println!("\tVersion: {version}");
                    println!("\tSerial number: {serial_number}");
                    println!("\tUUID: {uuid:?}");
                    println!("\tSKU: {sku}");
                    println!("\tFamily: {family}");
                }
            }
        }
    }

    #[cfg(target_arch = "x86")]
    {
        let id = CpuId::default();
        let brand = id.get_processor_brand_string().unwrap();

        println!("Processor: {brand}", brand=brand.as_str());
    }

    Ok(())
}
