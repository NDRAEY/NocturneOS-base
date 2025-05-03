#![no_std]
#![no_main]

#![allow(unused_imports)]
#![allow(static_mut_refs)]

extern crate alloc;

use core::{arch::asm, cell::OnceCell, panic::PanicInfo};

pub mod audio;
pub mod drv;
pub mod gfx;
pub mod shell;
pub mod std;
pub mod system;

pub use noct_iso9660;
pub use noct_noctfs;
pub use noct_psf;
use noct_psf::PSF;
pub use noct_tarfs;

use noct_alloc::Allocator;
pub use noct_logger::*;
use noct_tty::println;

pub use noct_ipc::manager::ipc_init;

#[global_allocator]
static ALLOCATOR: Allocator = noct_alloc::Allocator;

#[panic_handler]
fn panic(_info: &PanicInfo) -> ! {
    qemu_err!("{}", _info);
    println!("{}", _info);

    loop {
        unsafe {
            asm!("hlt");
        }    
    }
}

#[no_mangle]
pub static mut PSF_FONT: OnceCell<PSF> = OnceCell::new();

#[no_mangle]
pub unsafe extern "C" fn psf_init(ptr: *const u8, len: u32) {
    unsafe {
        let psf = PSF::from_raw_ptr(ptr, len as _).expect("Failed to initialize PSF font.");

        PSF_FONT.set(psf).unwrap()
    };

    qemu_note!("Font initialized!");
}

/// Main entry point for testing.
#[no_mangle]
#[inline(never)]
pub extern "C" fn rust_main() {
    println!("Привет, {}!", "Rust");

    let smbios = noct_smbios::find_smbios().unwrap();

    qemu_note!("SMBIOS: {smbios:x?}");

    for i in smbios.scan() {
        qemu_note!("{i:?}");
    }

    // let mut p = Path::from_path("R:/").unwrap();
    // qemu_log!("{:?}", p);
    // qemu_log!("{:?}", p.apply(".."));

    // p.apply("1/2/../3/.././4/5/6"); // 1/4/5/6
    // qemu_log!("{:?}", p);

    // crate::std::thread::spawn(move || {
    //     for i in (1..=16) {
    //         qemu_ok!("{}", i);
    //     }
    // })

    // {
    //     unsafe { noct_sched::scheduler_mode(false) };

    //     noct_screen::fill(0);

    //     let screen_dimen = noct_screen::dimensions();
    //     let mut tty = noct_tty::console::Console::new(screen_dimen.1 / 16, screen_dimen.0 / 8);

    //     tty.print_str("Ninja-go!\n");
    //     tty.print_str("Zane is the current Elemental Master and Ninja of Ice, P.I.X.A.L.'s boyfriend, as well as the first Nindroid. Zane was recruited by Master Wu, and learned Spinjitzu and discovered his other teammates, with whom he embarked on many adventures; battling the likes of the Serpentine and the Stone Army. During this time, Zane was reunited with his inventor/father figure, Dr. Julien, but sometime after the Final Battle, Dr. Julien passed away again. After Lloyd won the Final Battle, Pythor aided the Overlord in becoming the Golden Master and commanding an army of Nindroids that were made from Zane's blueprints. During this time, Zane grew closer to another droid, P.I.X.A.L., and battled a Nindroid named Cryptor. The ninja clashed with the Nindroids until Zane sacrificed himself to defeat the Golden Master. After building himself a new body and before he could go back to his friends, Zane was captured by Chen, and sent to a mysterious island. The ninja eventually found Zane in his new titanium body, (with Cole being the first) and rescued him. They then allied with the Elemental Masters to defeat the Anacondrai Cultists. With Zane back in the action, the ninja went on to battle Morro and an army of ghosts. After Nya destroyed the Cursed Realm, the ninja fought Nadakhan, who rebuilt his home realm in Ninjago, though this was undone by Jay's final wish. On the Day of the Departed, Cole accidentally revived Cryptor (and many other deceased villains), who Zane fought and destroyed once more. When the Time Twins and Vermillion warriors fought the ninja, Zane helped the ninja defeat their adversaries, but lost contact with P.I.X.A.L. in the chaos.");

    //     loop {
    //         let start_time = timestamp();

    //         noct_screen::fill(0);
    //         noct_tty::renderer::render(&tty);
    //         noct_screen::flush();

    //         qemu_log!("Took: {} ms", timestamp() - start_time);
    //     }
    // }

    // {
    //     let chan = create_named_channel("postman_guy").unwrap();

    //     chan.write("Hello!\0".as_bytes());

    //     spawn(move || {
    //         let my_chan = find_named_channel("postman_guy").unwrap();

    //         let mut data = [0u8; 6];

    //         my_chan.read(&mut data);

    //         qemu_ok!("Data is: {data:?}");
    //     });
    // }

    let program = noct_elfloader::load_elf_file("R:/test_h");

    if let Ok(mut program) = program {
        qemu_ok!("Running program...");

        program.run(&[]);
    }
}
