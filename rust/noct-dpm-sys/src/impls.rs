use crate::{dpm_read, dpm_write, DPM_Disk, DPM_Disks};

pub struct Disk {
    // disk: *mut DPM_Disk,
    letter: char,
}

impl Disk {
    pub fn read(&self, offset_high: u64, offset_low: u64, size: usize, out: &mut [u8]) -> usize {
        unsafe {
            dpm_read(
                self.letter as _,
                offset_high,
                offset_low,
                size as _,
                out.as_mut_ptr() as *mut _,
            ) as usize
        }
    }

    pub fn write(&self, offset_high: u64, offset_low: u64, size: usize, out: &[u8]) -> usize {
        unsafe {
            dpm_write(
                self.letter as _,
                offset_high,
                offset_low,
                size as _,
                out.as_ptr() as *const _,
            ) as usize
        }
    }
}

pub const fn get_disk(letter: char) -> Option<Disk> {
    let letter = letter.to_ascii_uppercase();

    let index = letter as i8 - 'A' as i8;
    let rdisk: &mut DPM_Disk = unsafe { &mut DPM_Disks[index as usize] };

    if !rdisk.Ready {
        return None;
    }

    Some(Disk {
        // disk: rdisk as *mut DPM_Disk,
        letter,
    })
}
