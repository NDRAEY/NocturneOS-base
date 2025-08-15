use alloc::string::String;
use num_derive::FromPrimitive;
use num_traits::FromPrimitive;

#[repr(u32)]
#[derive(FromPrimitive, Debug, Clone, Copy)]
pub enum DriveType {
    Unknown = 0xff,

    HardDrive = 0x00,
    OpticalDrive = 0x01,
    RemovableMedia = 0x02,
}

#[repr(u32)]
#[derive(FromPrimitive, Debug, Clone, Copy)]
pub enum MediumStatus {
    Offline = 0x00,
    Loading = 0x01,
    Online = 0x02
}

#[repr(u32)]
#[derive(FromPrimitive, Debug, Clone, Copy)]
pub enum Command {
    /// Eject the medium.
    /// No command parameters supplied.
    /// No return data expected.
    Eject = 0x00,
    /// Get medium status.
    /// No command parameters supplied.
    /// Returns 4 bytes representing an `u32` as MediumStatus.
    GetMediumStatus = 0x01,
    /// Get drive type.
    /// No command parameters supplied.
    /// Returns 4 bytes representing an `u32` as DriveType.
    GetDriveType = 0x02,
    /// Get medium capacity.
    /// No command parameters supplied.
    /// Returns 12 bytes:
    /// - bytes 0..8 representing an `u64` as actual capacity in sectors.
    /// - bytes 8..12 representing an `u32` as block size.
    GetMediumCapacity = 0x03,
}

pub trait Drive {
    fn get_name(&self) -> &str;
    fn get_id(&self) -> &str;

    fn get_parent_disk_id(&self) -> Option<&str> {
        None
    }

    fn get_type(&mut self) -> DriveType {
        let mut data = [0u8; 4];

        self.control(Command::GetDriveType, &[], &mut data);

        DriveType::from_u32(u32::from_ne_bytes(data)).unwrap_or(DriveType::Unknown)
    }

    fn get_capacity(&mut self) -> Option<u64> {
        let mut data = [0u8; 12];

        self.control(Command::GetMediumCapacity, &[], &mut data);

        match u64::from_ne_bytes(data[..8].try_into().unwrap()) {
            0 => None,
            n => Some(n)
        }
    }

    fn get_block_size(&mut self) -> Option<u32> {
        let mut data = [0u8; 12];

        self.control(Command::GetMediumCapacity, &[], &mut data);

        match u32::from_ne_bytes(data[8..].try_into().unwrap()) {
            0 => None,
            n => Some(n)
        }
    }

    fn read(&mut self, location: u64, buffer: &mut [u8]) -> i64;
    fn write(&mut self, location: u64, buffer: &[u8]) -> i64;
    fn control(&mut self, command: Command, command_parameters: &[u8], data: &mut [u8]) -> i64;
}