#![no_std]
#![allow(non_upper_case_globals)]
#![allow(non_camel_case_types)]
#![allow(non_snake_case)]

pub mod impls;
pub use impls::*;

include!(concat!(env!("OUT_DIR"), "/bindings.rs"));
