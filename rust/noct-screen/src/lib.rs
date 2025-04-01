#![no_std]
#![allow(non_upper_case_globals)]
#![allow(non_camel_case_types)]
#![allow(non_snake_case)]

use core::cell::OnceCell;

use screen::Screen;

include!(concat!(env!("OUT_DIR"), "/bindings.rs"));

pub mod c_api;
pub mod screen;

pub static mut GLOBAL_SCREEN: OnceCell<Screen<'static>> = OnceCell::new();

pub fn get_screen() -> Option<&'static mut Screen<'static>> {
    let mgr_ref = unsafe { &mut GLOBAL_SCREEN };

    mgr_ref.get_mut()
}