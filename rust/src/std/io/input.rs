use alloc::{string::String, vec::Vec};

use crate::std::io::screen::screen_update;
use noct_tty::print;

extern "C" {
    pub fn getchar() -> u32;
}

pub fn read_to_string() -> String {
    let mut st = Vec::<u8>::with_capacity(16);

    loop {
        let ch = unsafe { getchar() };

        if ch == '\n' as u32 {
            break;
        }

        if ch == 8 {
            if st.pop().is_some() {
                print!("{0} {0}", char::from_u32(8).unwrap());
            }
            continue;
        }

        st.push((ch & 0xff) as u8);

        if ch > 255 {
            st.push((ch >> 8) as u8);
        }

        print!("{}", char::from_u32(ch).unwrap());

        unsafe {
            screen_update();
        }
        // tty_printf("%c", ch);
    }

    String::from_utf8_lossy(&st).into_owned()
}
