#![no_std]

extern crate alloc;

use core::cell::RefCell;

use alloc::{
    format,
    string::{String, ToString},
};
use gi_ui::{canvas::Canvas, components::text8x8::Text, draw::Draw, size::Size};
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

struct Pavi {
    filepath: String,
    image: Image,
    render_mode: RefCell<ShowMode>,
}

impl Pavi {
    pub fn new(fpath: &String) -> Result<Self, String> {
        let data = match noct_fs::read(fpath) {
            Ok(x) => x,
            Err(e) => return Err(e.to_string()),
        };

        let image = match nimage::tga::from_tga_data(data.as_slice()) {
            Some(x) => x,
            None => return Err("Invalid file format.".to_string()),
        };

        Ok(Self {
            filepath: fpath.to_string(),
            image,
            render_mode: RefCell::new(ShowMode::BoundsX),
        })
    }

    fn render_image(&self) {
        let width: usize;
        let height: usize;
        let (mut start_x, mut start_y) = (0isize, 0isize);

        let (scr_w, scr_h) = noct_screen::dimensions();

        let im_w = self.image.width();
        let im_h = self.image.height();

        // qemu_note!("Screen: ({scr_w}, {scr_h}); Image: ({im_w}, {im_h})");

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
                if im_w < scr_w {
                    start_x = (scr_w as isize - im_w as isize) / 2;
                }

                if im_h < scr_h {
                    start_y = (scr_h as isize - im_h as isize) / 2;
                }

                (width, height) = (im_w, im_h);
            }
        }

        let new_image = self.image.scale_to_new(width, height);

        let mut text = Text::new()
            .with_color(0xff_00ff00)
            .with_kerning(1)
            .with_size(12)
            .with_text(format!(
                "{} - [{}x{}] ({:?} | {:?}) ({scr_w}x{scr_h})",
                self.filepath,
                self.image.width(),
                self.image.height(),
                self.render_mode.borrow(),
                self.image.pixel_format()
            ));

        let mut canvas = Canvas::new(text.size().0, text.size().1);

        text.draw(&mut canvas, 0, 0);

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

        let csz = canvas.size();

        for x in 0..csz.0 {
            for y in 0..csz.1 {
                let pixel = canvas.get_pixel(x, y).unwrap();

                if (pixel & 0xFF000000) == 0 {
                    continue;
                }

                noct_screen::set_pixel(x, y, pixel);
            }
        }

        noct_screen::flush();
    }

    pub fn run(&self) {
        self.render_image();

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

pub fn pavi(argv: &[String]) -> Result<(), usize> {
    let filename = argv.first();

    if filename.is_none() {
        println!("Provide a file!");
        println!("Usage: pavi <filename>");

        return Err(1);
    }

    let filename = filename.unwrap();

    let pavi = Pavi::new(filename);

    if let Err(e) = pavi {
        println!("{filename}: {e}");
        return Err(1);
    }

    let pavi = pavi.unwrap();

    pavi.run();

    Ok(())
}
