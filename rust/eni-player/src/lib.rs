#![no_std]

extern crate alloc;

use core::cell::RefCell;

use alloc::boxed::Box;
use alloc::{format, vec};
use alloc::string::{String, ToString};
use alloc::sync::Arc;
use alloc::vec::Vec;
use border_wrapped::BorderWrapped;
use embedded_canvas::Canvas;
use embedded_graphics::Drawable;
use embedded_graphics::mono_font::ascii;
use embedded_graphics::prelude::{Transform, WebColors};
use embedded_graphics::primitives::{PrimitiveStyle, Rectangle, StyledDrawable, Triangle};
use embedded_graphics::text::{Baseline, Text, TextStyle};
use embedded_graphics::{
    mono_font::MonoTextStyle,
    pixelcolor::Rgb888,
    prelude::{Point, RgbColor, Size},
};
use embedded_layout::layout;
use embedded_layout::layout::linear::spacing::DistributeFill;
use embedded_layout::layout::linear::{LinearLayout, spacing};
use embedded_layout::prelude::Chain;
use margin::Margin;
use noct_fs::File;
use noct_input::kbd::{Key, SpecialKey};
use noct_logger::{qemu_log, qemu_note, qemu_ok};
use noct_sched::{spawn, task_yield};
use noct_tty::println;
use spin::Mutex;

const CACHE_SIZE: usize = 128 << 10;

mod border_wrapped;
mod margin;
mod text_list;

#[derive(PartialEq)]
pub enum PlayerStatus {
    Playing,
    Paused,
    Stop,
}

fn draw_status(status: &PlayerStatus, canvas: &mut Canvas<Rgb888>) {
    let start_point_x = 8;
    let start_point_y = 130;
    let step = 6;
    let style = PrimitiveStyle::with_fill(Rgb888::CSS_AQUA);

    match status {
        PlayerStatus::Playing => {
            let tria = Triangle::new(
                Point::new(start_point_x, start_point_y),
                Point::new(start_point_x + (step * 2), start_point_y + step),
                Point::new(start_point_x, start_point_y + (step * 2)),
            );

            tria.draw_styled(&style, canvas).unwrap();
        }
        PlayerStatus::Paused => {
            let rect1 = Rectangle::new(
                Point::new(start_point_x, start_point_y),
                Size::new(step as _, (step * 2) as _),
            );

            let rect2 = Rectangle::new(
                Point::new(start_point_x + (step * 2), start_point_y),
                Size::new(step as _, (step * 2) as _),
            );

            rect1.draw_styled(&style, canvas).unwrap();
            rect2.draw_styled(&style, canvas).unwrap();
        }
        PlayerStatus::Stop => {
            Rectangle::new(
                Point::new(start_point_x, start_point_y),
                Size::new((step * 2) as _, (step * 2) as _),
            )
            .draw_styled(&style, canvas)
            .unwrap();
        }
    }
}

fn draw_ui(
    canvas: &mut Canvas<Rgb888>,
    status: &PlayerStatus,
    filename: &str,
    cached_size: &usize,
    current_time: &usize,
    total_time: &usize,
) {
    canvas.pixels.fill(None);

    let header = format!("Eni {} by NDRAEY", env!("CARGO_PKG_VERSION"));

    let font = MonoTextStyle::new(&ascii::FONT_10X20, Rgb888::WHITE);

    let ssg = eg_seven_segment::SevenSegmentStyleBuilder::new()
        .digit_size(Size::new(24, 48))
        .digit_spacing(10)
        .segment_width(6)
        .segment_color(Rgb888::CSS_AQUA)
        .build();

    let title = Text::with_text_style(
        &header,
        Point::zero(),
        font,
        TextStyle::with_baseline(Baseline::Top),
    );

    let track_info = format!("{}\n\n{}", filename, "Unknown artist - Unknown");

    let track_info = Margin::new(Text::with_text_style(
        &track_info,
        Point::zero(),
        font,
        TextStyle::with_baseline(Baseline::Top),
    ))
    .top(15);

    let curtime = format!("{:<02}:{:<02}", current_time / 60, current_time % 60);
    let tottime = format!("{:<02}:{:<02}", total_time / 60, total_time % 60);

    let time_current = Margin::new(Text::with_text_style(
        &curtime,
        Point::zero(),
        ssg,
        TextStyle::with_baseline(Baseline::Top),
    ))
    .left(35)
    .top(25)
    .bottom(25)
    .right(25);

    let time_total = Margin::new(Text::with_text_style(
        &tottime,
        Point::zero(),
        {
            let mut new = ssg.clone();

            new.digit_size.width -= new.digit_size.width / 4;
            new.digit_size.height -= new.digit_size.height / 3;

            new.segment_width -= 2;

            new
        },
        TextStyle::with_baseline(Baseline::Top),
    ))
    .left(10)
    .bottom(25);

    let footer = Text::with_text_style(
        "L = show track list; Space = Play / Pause;\n<- = 5 seconds rewind; -> = 5 seconds forward",
        Point::zero(),
        font,
        TextStyle::with_baseline(Baseline::Top),
    );

    let cached_size = format!("Cached: {} kb", (*cached_size) >> 10);

    let cache_meter = Text::with_text_style(
        &cached_size,
        Point::zero(),
        {
            let mut new = font.clone();
            new.background_color = Some(Rgb888::WHITE);
            new.text_color = Some(Rgb888::BLACK);
            new
        },
        TextStyle::with_baseline(Baseline::Top),
    );

    let layout = LinearLayout::vertical(
        Chain::new(
            LinearLayout::vertical(Chain::new(title).append(track_info).append(
                LinearLayout::horizontal(Chain::new(time_current).append(time_total)).arrange(),
            ))
            .arrange(),
        )
        .append(cache_meter)
        .append(footer),
    )
    .with_spacing(spacing::DistributeFill(canvas.canvas.height))
    .arrange();

    draw_status(status, canvas);

    layout.draw(canvas).unwrap();
}

fn render_canvas(canvas: &mut Canvas<Rgb888>) {
    let mut pixels = canvas
        .pixels
        .iter()
        .map(|a| a.unwrap_or(Rgb888::BLACK))
        .map(|x| ((x.r() as u32) << 16) | ((x.g() as u32) << 8) | ((x.b()) as u32));

    for y in 0..600 {
        for x in 0..800 {
            noct_screen::set_pixel(x, y, pixels.next().unwrap());
        }
    }
}

pub fn player(args: &[String]) -> Result<(), usize> {
    let filepath = match args.get(0) {
        Some(fp) => fp,
        None => {
            println!("No arguments!");
            return Err(1);
        },
    };

    let mut audio = noct_audio::get_device(0).unwrap();
    audio.set_rate(48000);

    let cache_line: Arc<Mutex<Vec<Box<[u8]>>>> = Arc::new(Mutex::new(Vec::new()));

    let mut file: Arc<Mutex<File>> = Arc::new(Mutex::new(noct_fs::File::open(&filepath).unwrap()));

    let mut is_running = Arc::new(Mutex::new(true));
    let mut status = PlayerStatus::Playing;
    let mut cached_size = Arc::new(Mutex::new(RefCell::new(0usize)));
    let mut bytes_read = 0;
    let mut current_time_seconds = 0;
    let mut total_time_seconds = 193;

    let filesize = file.lock().size();

    let mut canvas: Canvas<Rgb888> = Canvas::new(Size::new(800, 600));

    let arced_running = is_running.clone();
    let arced_file = file.clone();
    let arced_cache = cache_line.clone();
    spawn(move || {
        while *arced_running.lock() {
            let mut file_bnd = arced_file.lock();
            let mut cache_bnd = arced_cache.lock();

            if bytes_read >= filesize {
                qemu_log!("Reached the end!");
                break;
            }

            if cache_bnd.len() > 5 {
                continue;
            }

            let datasize = core::cmp::min(CACHE_SIZE, filesize - bytes_read);
            let mut buffer = vec![0u8; datasize];
            
            file_bnd.read(buffer.as_mut()).unwrap();
            cache_bnd.push(buffer.into_boxed_slice());

            bytes_read += CACHE_SIZE;

            task_yield();
        }

        qemu_ok!("Exit!");
    });

    loop {
        let key = unsafe { noct_input::keyboard_buffer_get_or_nothing() };
        let (key, is_pressed) = noct_input::kbd::parse_scancode(key as u8).unwrap();
        
        if key == Key::Special(SpecialKey::ESCAPE) {
            noct_screen::fill(0);
            break;
        } else if key == Key::Character(' ') && is_pressed {
            status = match status {
                PlayerStatus::Playing => PlayerStatus::Paused,
                _ => PlayerStatus::Playing
            };

            continue;
        }

        {
            if status == PlayerStatus::Playing {
                let mut x = cache_line.lock();
                let block = x.get(0).map(|a| a.clone());
                match block {
                    Some(a) => {
                        audio.write(&a);

                        x.remove(0);

                        qemu_log!("Removed! {}", x.len());
                    },
                    None => (),
                };
            }
        }

        draw_ui(
            &mut canvas,
            &status,
            &filepath,
            &cache_line.lock().iter().map(|a| a.len()).sum(),
            &current_time_seconds,
            &total_time_seconds,
        );

        render_canvas(&mut canvas);
    }

    audio.close();

    *is_running.lock() = false;

    Ok(())
}
