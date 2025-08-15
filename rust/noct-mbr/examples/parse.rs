use std::io::Read;

use noct_mbr::parse_from_sector;

fn main() {
    let filename = match std::env::args().skip(1).next() {
        None => {
            eprintln!("Do `sudo dd if=/dev/sda bs=512 count=1 of=mbr.bin` then pass file here");
            std::process::exit(1);
        },
        Some(x) => x
    };

    let data = {
        let mut buffer = vec![0u8; 512];

        let mut file = std::fs::OpenOptions::new().read(true).open(filename).unwrap();

        file.read(&mut buffer).unwrap();

        buffer
    };

    let records = parse_from_sector(&data);

    for i in records {
        println!("{i:x?} => {:x?} {:x?}", i.start_chs(), i.end_chs());
    }
}