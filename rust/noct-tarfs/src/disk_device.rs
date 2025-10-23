use core::ffi::c_char;

use no_std_io::io::{Read, Seek, Write};

use crate::{raw_ptr_to_str};
// use noct_dpm_sys::Disk;

pub struct DiskDevice<'dname> {
    pub disk: &'dname str,
    position: u64,
}

impl DiskDevice<'_> {
    pub fn new(disk: *const c_char) -> Self {
        DiskDevice {
            disk: unsafe { raw_ptr_to_str(disk) },
            position: 0,
        }
    }
}

impl Read for DiskDevice<'_> {
    fn read(&mut self, buffer: &mut [u8]) -> no_std_io::io::Result<usize> {
        let read_size = noct_diskman::read(&self.disk, self.position as _, buffer);

        if read_size != -1 {
            self.position += read_size as u64;
        }

        Ok(read_size as _)
    }
}

impl Write for DiskDevice<'_> {
    fn write(&mut self, buffer: &[u8]) -> no_std_io::io::Result<usize> {
        let size = noct_diskman::write(&self.disk, self.position as _, buffer);

        if size != -1 {
            self.position += size as u64;
        }

        Ok(size as _)
    }

    fn flush(&mut self) -> no_std_io::io::Result<()> {
        // ...

        Ok(())
    }
}

impl Seek for DiskDevice<'_> {
    fn seek(&mut self, pos: no_std_io::io::SeekFrom) -> no_std_io::io::Result<u64> {
        match pos {
            no_std_io::io::SeekFrom::Start(pos) => {
                self.position = pos;
            }
            no_std_io::io::SeekFrom::End(_pos) => {
                todo!(
                    "Implement getting info about disk and use disk's max position to determine offset against disk's end"
                );
            }
            no_std_io::io::SeekFrom::Current(pos) => {
                self.position = (self.position as i64 + pos).try_into().unwrap();
            }
        };

        Ok(self.position)
    }
}

impl tarfs::Device for DiskDevice<'_> {}
