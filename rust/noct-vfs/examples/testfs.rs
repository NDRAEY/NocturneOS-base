use nocturne_vfs::{File, Filesystem, FilesystemError, Result};

#[derive(Debug)]
struct TestFS {
    // ...
}

#[derive(Debug)]
struct TestFSFile {
    // ...
}

impl File for TestFSFile {
    fn read(&self, buffer: &mut [u8], count: usize, size: usize) -> Result<()> {
        for (nr, i) in (0..(count*size)).enumerate() {
            buffer[nr] = (i % 256) as u8;
        }

        Ok(())
    }
    
    fn write(&mut self, buffer: &[u8], count: usize, size: usize) -> Result<()> {
        todo!()
    }
    
    fn seek(&mut self, position: nocturne_vfs::SeekPosition) {
        todo!()
    }
    
    fn tell(&self) -> usize {
        todo!()
    }
}

impl Filesystem for TestFS {
    #[allow(refining_impl_trait)]
    fn open(&self, path: &str) -> Result<TestFSFile> {
        println!("Open file: {}", &path);

        Ok(TestFSFile {})
    }

    fn create(&mut self, path: &str) -> Result<()> {
        todo!()
    }
    
    fn delete(&mut self, path: &str) -> Result<()> {
        todo!()
    }
    
    fn info(&self, path: &str) -> Result<nocturne_vfs::DirectoryEntry> {
        todo!()
    }
    
    fn list(&self, path: &str) -> Result<Vec<nocturne_vfs::DirectoryEntry>> {
        todo!()
    }
    
    fn probe(&mut self) -> bool {
        todo!()
    }
}

fn main() {
    let mut fs = TestFS {};
    fs.probe();

    let mut buffer = [0u8; 8];

    let file = fs.open("/hello.txt").unwrap();
    file.read_all(&mut buffer).unwrap();

    println!("{:?}", buffer);
}