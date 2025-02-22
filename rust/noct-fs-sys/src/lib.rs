#![no_std]

use noct_alloc::Allocator;

#[global_allocator]
static ALLOCATOR: Allocator = noct_alloc::Allocator;

pub mod types;