#[cfg(target_arch = "x86")]
use raw_cpuid::CpuId;

// Used by SMBIOS printing branch, which is only available on x86(_64)
#[cfg(target_arch = "x86")]
use noct_tty::print;

use noct_tty::println;

use super::ShellContext;

pub static SYSINFO_COMMAND_ENTRY: crate::ShellCommandEntry =
    ("sysinfo", sysinfo, Some("System info"));

pub fn sysinfo(_context: &mut ShellContext, _args: &[&str]) -> Result<(), usize> {
    let (maj, min, patch) = noct_sys::version::version();
    let arch = noct_sys::version::architecture();
    let vername = noct_sys::version::version_name();

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
                    print!("\tSegment Address: {segment_address:x}");
                    println!("; ROM size: {rom_size}");
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
                noct_smbios::SMBIOSEntry::Processor {
                    socket_designation,
                    processor_type,
                    processor_family,
                    processor_manufacturer,
                    processor_id,
                    processor_version,
                    voltage,
                    external_clock,
                    max_speed,
                    current_speed,
                } => {
                    println!("Processor:");
                    println!("\tSocket designation: {socket_designation}");
                    println!("\tProcessor type: {processor_type}");
                    println!("\tProcessor family: {processor_family}");
                    println!("\tProcessor manufacturer: {processor_manufacturer}");
                    println!("\tProcessor ID: {processor_id}");
                    println!("\tProcessor version: {processor_version}");
                    println!("\tVoltage: {voltage} V");
                    println!("\tExternal clock: {external_clock} MHz");
                    print!("\tCurrent speed: {current_speed} MHz");
                    println!("; Max speed: {max_speed} MHz");
                }
                noct_smbios::SMBIOSEntry::MemoryDevice {
                    memory_manufacturer,
                    size,
                    memory_speed,
                } => {
                    println!("Memory Device:");
                    println!("\tMemory manufacturer: {memory_manufacturer}");
                    println!("\tSize: {size} MB");
                    println!("\tMemory speed: {memory_speed} MT/s");
                }
            }
        }
    }

    #[cfg(target_arch = "x86")]
    {
        let id = CpuId::default();
        let brand = id.get_processor_brand_string().unwrap();

        println!("Processor: {brand}", brand = brand.as_str());
    }

    Ok(())
}
