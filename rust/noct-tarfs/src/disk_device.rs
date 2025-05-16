use noct_dpm_sys::Disk;
use no_std_io::io::{Read, Seek};

pub struct DiskDevice {
    disk: Disk,
    position: u64
}

impl DiskDevice {
    pub const fn new(disk: Disk) -> Self {
        DiskDevice {
            disk,
            position: 0
        }
    }
}

impl Read for DiskDevice {
    fn read(&mut self, buf: &mut [u8]) -> no_std_io::io::Result<usize> {
        let read_size = self.disk.read(0, self.position, buf.len(), buf);

        Ok(read_size)
    }
}

impl Seek for DiskDevice {
    fn seek(&mut self, pos: no_std_io::io::SeekFrom) -> no_std_io::io::Result<u64> {
        match pos {
            no_std_io::io::SeekFrom::Start(pos) => {
                self.position = pos;
            },
            no_std_io::io::SeekFrom::End(_pos) => {
                // let disk_size = self.disk.size();
                // self.position = disk_size + pos;
                todo!("Implement getting info about disk and use disk's max position to determine offset against disk's end");
            }
            no_std_io::io::SeekFrom::Current(pos) => {
                self.position = (self.position as i64 + pos).try_into().unwrap();
            }
        };

        Ok(self.position)
    }
}