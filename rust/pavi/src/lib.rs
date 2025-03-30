#![no_std]

extern crate alloc;

use alloc::string::{String, ToString};
use nimage::Image;
use noct_input::keyboard_buffer_get_or_nothing;
use noct_logger::qemu_log;
use noct_tty::println;

enum ShowMode {
    BoundsX,
    BoundsY,
    Stretch
}

pub fn pavi(argc: usize, argv: &[String]) -> Result<(), usize> {
    let filename = argv.iter().skip(1).last();

    if filename.is_none() {
        println!("Failed to open file!");
        return Err(1);
    }

    let filename = filename.unwrap();

    let data = noct_fs::read(filename);

    if let Err(e) = data {
        println!("{}: {}", filename, e.to_string());
        return Err(1);
    }
    
    let data = data.unwrap();


    let image = nimage::tga::from_tga_data(data.as_slice());

    if image.is_none() {
        println!("{}: Invalid file format.", filename);
        return Err(1);
    }

    let image = image.unwrap();

    let (screen_width, screen_height) = noct_screen::dimensions();
    // noct_screen::set_pixel(x, y, color);


    render_image(&image, ShowMode::BoundsX);
    
    while unsafe { keyboard_buffer_get_or_nothing() } != 1 {
        core::sync::atomic::compiler_fence(core::sync::atomic::Ordering::SeqCst);
    }

    Ok(())
}

fn render_image(image: &Image, render_mode: ShowMode) {
    let (mut start_x, mut start_y, mut width, mut height) = (0,0, 0,0);

    match render_mode {
        ShowMode::BoundsX => {
            let im_w = image.width();
            let im_h = image.height();

            let (scr_w, scr_h) = noct_screen::dimensions();

            width = scr_w;

            let diff = im_w as f64 / scr_w as f64;

            height = (im_h as f64 / diff) as usize;

            start_y = (scr_h - height) / 2;
        },
        ShowMode::BoundsY => {
            todo!()
        },
        ShowMode::Stretch => {
            (width, height) = noct_screen::dimensions();
        },
    }

    let mut new_image = image.clone();
    new_image.scale(width, height);

    for x in 0..width {
        for y in 0..height {
            let pixel = new_image.get_pixel(x, y);

            let mut pixel = pixel.unwrap_or(0) as u32;

            pixel = ((pixel & 0xff0000) >> 16) | ((pixel & 0x00ff00)) | ((pixel & 0x0000ff) << 16);

            noct_screen::set_pixel((start_x + x) as u32, (start_y + y) as u32, pixel);
        }
    }
}