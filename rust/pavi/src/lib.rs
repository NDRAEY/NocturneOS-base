#![no_std]

extern crate alloc;

use core::cell::RefCell;

use alloc::{
    format,
    string::{String, ToString},
};
use embedded_canvas::Canvas;
use embedded_graphics::{
    Drawable,
    mono_font::{
        MonoTextStyle,
        ascii::FONT_8X13_BOLD,
    },
    pixelcolor::Rgb888,
    prelude::{Dimensions, Point, RgbColor, Size},
    text::{Baseline, Text},
};
use nimage::Image;
use noct_input::{
    kbd::{Key, SpecialKey, parse_scancode},
    keyboard_buffer_get,
};
use noct_logger::{qemu_err, qemu_note};
use noct_tty::println;

#[derive(Debug)]
enum ShowMode {
    Centered,
    BoundsX,
    BoundsY,
    Stretch,
}

struct Pavi<'a> {
    filepath: String,
    image: Image,
    render_mode: RefCell<ShowMode>,

    text_ch_style: MonoTextStyle<'a, Rgb888>,
    show_status: bool,
}

impl Pavi<'_> {
    pub fn new(fpath: &str) -> Result<Self, String> {
        let data = match noct_fs::read(fpath) {
            Ok(x) => x,
            Err(e) => return Err(e.to_string()),
        };

        let Some(image) = nimage::tga::from_tga_data(&data) else {
            return Err("Invalid file format.".to_string());
        };

        Ok(Self {
            filepath: fpath.to_string(),
            image,
            render_mode: RefCell::new(ShowMode::BoundsX),
            text_ch_style: MonoTextStyle::new(&FONT_8X13_BOLD, Rgb888::new(0, 0xcc, 0)),
            show_status: true,
        })
    }

    fn render_image(&self) {
        let width: usize;
        let height: usize;
        let (mut start_x, mut start_y) = (0isize, 0isize);

        let (scr_w, scr_h) = noct_screen::dimensions();

        let im_w = self.image.width();
        let im_h = self.image.height();

        match *self.render_mode.borrow() {
            ShowMode::BoundsX => {
                let diff = im_w as f64 / scr_w as f64;

                width = scr_w;
                height = (im_h as f64 / diff) as usize;

                start_y = (scr_h as isize - height as isize) / 2;
            }
            ShowMode::BoundsY => {
                let diff = im_h as f64 / scr_h as f64;

                width = (im_w as f64 / diff) as usize;
                height = scr_h;

                start_x = (scr_w as isize - width as isize) / 2;
            }
            ShowMode::Stretch => {
                (width, height) = (scr_w, scr_h);
            }
            ShowMode::Centered => {
                start_x = (scr_w as isize - im_w as isize) / 2;
                start_y = (scr_h as isize - im_h as isize) / 2;

                (width, height) = (im_w, im_h);
            }
        }

        qemu_note!("Screen: ({scr_w}, {scr_h}); Image: ({im_w}, {im_h})");
        qemu_note!("{start_x} {start_y} {width} {height}");

        let new_image = self.image.scale_to_new(width, height);

        noct_screen::fill(0);

        for x in 0..width {
            for y in 0..height {
                let pixel = new_image.get_pixel(x, y);

                let mut pixel = pixel.unwrap_or(0);

                if pixel == 0 {
                    continue;
                }

                pixel =
                    ((pixel & 0xff0000) >> 16) | (pixel & 0x00ff00) | ((pixel & 0x0000ff) << 16);

                let rx = start_x + x as isize;
                let ry = start_y + y as isize;

                if rx < 0 || ry < 0 {
                    continue;
                }

                noct_screen::set_pixel(rx as usize, ry as usize, pixel);
            }
        }

        if self.show_status {
            let fmted = format!(
                "{} - [{}x{}] ({:?} | {:?}) ({scr_w}x{scr_h})",
                self.filepath,
                self.image.width(),
                self.image.height(),
                self.render_mode.borrow(),
                self.image.pixel_format()
            );

            let text =
                Text::with_baseline(&fmted, Point::new(0, 0), self.text_ch_style, Baseline::Top);

            let sz = text.bounding_box().size;
            let mut canvas: Canvas<Rgb888> = Canvas::new(Size::new(sz.width, sz.height));

            text.draw(&mut canvas).unwrap();

            for x in 0..sz.width {
                for y in 0..sz.height {
                    let coord = (y * sz.width) + x;
                    let pixel = canvas.pixels[coord as usize]
                        .map(|x| ((x.r() as u32) << 16) | ((x.g() as u32) << 8) | (x.b() as u32))
                        .unwrap_or(0);

                    noct_screen::set_pixel(x as _, y as _, pixel);
                }
            }
        }

        noct_screen::flush();
    }

    pub fn run(&mut self) {
        self.render_image();

        loop {
            let ckey = keyboard_buffer_get();
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
                Key::Character('s') => {
                    self.show_status = !self.show_status;

                    self.render_image();
                }
                Key::Character('t') => {
                    let next_mode = match *self.render_mode.borrow() {
                        ShowMode::BoundsX => ShowMode::BoundsY,
                        ShowMode::BoundsY => ShowMode::Stretch,
                        ShowMode::Stretch => ShowMode::Centered,
                        ShowMode::Centered => ShowMode::BoundsX,
                    };

                    *self.render_mode.borrow_mut() = next_mode;

                    self.render_image();
                }
                _ => {
                    qemu_note!("Key {:?} is not supported yet", key);
                }
            }
        }
    }
}

pub fn pavi(argv: &[&str]) -> Result<(), usize> {
    let filename = match argv.first() {
        Some(fl) => fl,
        None => {
            println!("Provide a file!");
            println!("Usage: pavi <filename>");

            return Err(1);
        }
    };

    let mut pavi = match Pavi::new(filename) {
        Ok(pavi) => pavi,
        Err(e) => {
            println!("{filename}: {e}");
            return Err(1);
        }
    };

    pavi.run();

    Ok(())
}
