#![no_std]

extern crate alloc;

use alloc::string::{String, ToString};
use nimage::Image;
use noct_input::{kbd::{parse_scancode, Key, SpecialKey}, keyboard_buffer_get};
use noct_logger::{qemu_err, qemu_log, qemu_note};
use noct_tty::println;

#[derive(Debug)]
enum ShowMode {
    Centered,
    BoundsX,
    BoundsY,
    Stretch
}

pub fn pavi(_argc: usize, argv: &[String]) -> Result<(), usize> {
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

    let mut render_mode = ShowMode::BoundsX;
    render_image(&image, &render_mode);
    
    loop {
        let ckey = unsafe { keyboard_buffer_get() };
        let key = parse_scancode(ckey as u8);

        if key.is_none() {
            qemu_err!("Failed to parse key: {ckey}");
            continue;
        }

        let (key, pressed) = key.unwrap();

        if !pressed {
            continue;
        }

        match key {
            Key::Special(SpecialKey::ESCAPE) => {
                break;
            }
            Key::Character('t') => {
                render_mode = match render_mode {
                    ShowMode::BoundsX => ShowMode::BoundsY,
                    ShowMode::BoundsY => ShowMode::Stretch,
                    ShowMode::Stretch => ShowMode::Centered,
                    ShowMode::Centered => ShowMode::BoundsX,
                };

                render_image(&image, &render_mode);
            }
            _ => {
                qemu_note!("Key {:?} is not supported yet", key);
            }
        }
    }

    Ok(())
}

fn render_image(image: &Image, render_mode: &ShowMode) {
    let (mut start_x, mut start_y, mut width, mut height) = (0isize, 0isize, 0, 0);

    let (scr_w, scr_h) = noct_screen::dimensions();

    let im_w = image.width();
    let im_h = image.height();

    qemu_note!("Screen: ({scr_w}, {scr_h}); Image: ({im_w}, {im_h})");

    match render_mode {
        ShowMode::BoundsX => {
            let diff = im_w as f64 / scr_w as f64;
            
            width = scr_w;
            height = (im_h as f64 / diff) as usize;

            start_y = (scr_h as isize - height as isize) / 2;
        },
        ShowMode::BoundsY => {
            let diff = im_h as f64 / scr_h as f64;

            width = (im_w as f64 / diff) as usize;
            height = scr_h;

            start_x = (scr_w as isize - width as isize) / 2;
        },
        ShowMode::Stretch => {
            (width, height) = (scr_w, scr_h);
        },
        ShowMode::Centered => {
            if im_w < scr_w  {
                start_x = (scr_w as isize - im_w as isize) / 2;
            }

            if im_h < scr_h {
                start_y = (scr_h as isize - im_h as isize) / 2;
            }

            (width, height) = (im_w, im_h);
        }
    }

    qemu_note!("Mode: {render_mode:?}; X: {start_x}; Y: {start_y}; W: {width}; Height: {height}");

    let mut new_image = image.clone();
    new_image.scale(width, height);

    noct_screen::fill(0);

    for x in 0..width {
        for y in 0..height {
            let pixel = new_image.get_pixel(x, y);

            let mut pixel = pixel.unwrap_or(0) as u32;

            if pixel == 0 {
                continue;
            }

            pixel = ((pixel & 0xff0000) >> 16) | ((pixel & 0x00ff00)) | ((pixel & 0x0000ff) << 16);

            let rx = start_x + x as isize;
            let ry = start_y + y as isize;

            if rx < 0 || ry < 0 {
                continue;
            }

            noct_screen::set_pixel(rx as usize, ry as usize, pixel);
        }
    }
}
