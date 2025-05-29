#![no_std]

extern crate alloc;

use alloc::boxed::Box;
use alloc::string::{String, ToString};
use alloc::sync::Arc;
use alloc::{format, vec};
use embedded_canvas::Canvas;
use embedded_graphics::Drawable;
use embedded_graphics::mono_font::ascii;
use embedded_graphics::prelude::WebColors;
use embedded_graphics::primitives::{PrimitiveStyle, Rectangle, StyledDrawable, Triangle};
use embedded_graphics::text::{Baseline, Text, TextStyle};
use embedded_graphics::{
    mono_font::MonoTextStyle,
    pixelcolor::Rgb888,
    prelude::{Point, RgbColor, Size},
};
use embedded_layout::layout::linear::{LinearLayout, spacing};
use embedded_layout::prelude::Chain;
use margin::Margin;
use noct_fs::File;
use noct_input::kbd::{Key, SpecialKey};
use noct_logger::{qemu_note, qemu_ok};
use noct_sched::{spawn, task_yield};
use noct_timer::timestamp;
use noct_tty::println;
use nwav::Chunk::{Format, List};
use spin::Mutex;

use alloc::collections::LinkedList;

const CACHE_SIZE: usize = 128 << 10;

mod margin;
// mod border_wrapped;
// mod text_list;

#[derive(Copy, Clone, PartialEq)]
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
    track_info: &(String, String),
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

    let track_info = format!("{}\n\n{} - {}", filename, track_info.0, track_info.1);

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
        "Space = Play / Pause",
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

#[inline(always)]
fn bytes_to_seconds(fmtdata: &nwav::Fmt, byte_count: usize) -> usize {
    byte_count
        / (fmtdata.sampling_rate
            * fmtdata.number_of_channels as u32
            * (fmtdata.bits_per_sample as u32 >> 3)) as usize
}

pub fn player(args: &[&str]) -> Result<(), usize> {
    let filepath = match args.first() {
        Some(fp) => fp,
        None => {
            println!("No arguments!");
            return Err(1);
        }
    };

    let audio = match noct_audio::get_device(0) {
        Some(dev) => dev,
        None => {
            println!("No available audio device found!");
            return Err(1);
        }
    };
    audio.set_rate(48000);

    let cache_line: Arc<Mutex<LinkedList<Box<[u8]>>>> = Arc::new(Mutex::new(LinkedList::new()));

    let file: Arc<Mutex<File>> = Arc::new(Mutex::new(noct_fs::File::open(filepath).unwrap()));

    let (fmtdata, metadata) = {
        let mut bytes = vec![0; 2048];

        file.lock().read(&mut bytes).unwrap();

        let wav = nwav::WAV::from_data(&bytes);

        let Some(Format(chunk)) = wav.read_chunk_by_name("fmt ") else {
            println!("Cannot read the `fmt ` chunk! Are you sure that's a WAV file?");

            return Err(2);
        };

        let meta = match wav.read_chunk_by_name("LIST") {
            Some(List(l)) => Some(l),
            _ => None,
        };

        (chunk, meta)
    };

    let filesize = file.lock().size();

    let is_running = Arc::new(Mutex::new(true));
    let status = Arc::new(Mutex::new(PlayerStatus::Playing));
    let bytes_read = Arc::new(Mutex::new(0));
    let mut bytes_played: usize = 0;
    let total_time_seconds: usize = bytes_to_seconds(&fmtdata, filesize);

    let mut canvas: Canvas<Rgb888> = Canvas::new(Size::new(800, 600));

    let arced_running = is_running.clone();
    let arced_file = file.clone();
    let arced_cache = cache_line.clone();
    let arced_br = bytes_read.clone();
    spawn(move || {
        while *arced_running.lock() {
            if *arced_br.lock() >= filesize {
                // qemu_log!("Reached the end!");
                task_yield();
                continue;
            }

            if arced_cache.lock().len() > 5 {
                task_yield();
                continue;
            }

            // qemu_note!("Cache fetch");

            let datasize = core::cmp::min(CACHE_SIZE, filesize - *arced_br.lock());
            let mut buffer = vec![0u8; datasize];

            let mut file_bnd = arced_file.lock();
            file_bnd.read(buffer.as_mut()).unwrap();
            // arced_cache.lock().push(buffer.into_boxed_slice());
            arced_cache.lock().push_back(buffer.into_boxed_slice());

            *arced_br.lock() += CACHE_SIZE;
        }

        qemu_ok!("Exit!");
    });

    let meta = metadata
        .map(|a| {
            let artist = a
                .iter()
                .filter(|x| x.0 == "IART")
                .map(|x| x.1.clone())
                .next()
                .unwrap_or("Unknown artist".to_string());

            let title = a
                .iter()
                .filter(|x| x.0 == "INAM")
                .map(|x| x.1.clone())
                .next()
                .unwrap_or("Unknown".to_string());

            (artist, title)
        })
        .unwrap_or(("Unknown artist".to_string(), "Unknown".to_string()));

    loop {
        let key = noct_input::keyboard_buffer_get_or_nothing();
        let (key, is_pressed) =
            noct_input::kbd::parse_scancode(key as u8).unwrap_or((Key::Unknown, false));

        if key == Key::Special(SpecialKey::ESCAPE) {
            noct_screen::fill(0);
            break;
        } else if key == Key::Character(' ') && is_pressed {
            let curstat = *status.lock();

            *status.lock() = match curstat {
                PlayerStatus::Stop => {
                    qemu_note!("STOP!");

                    file.lock().rewind();
                    bytes_played = 0;
                    *bytes_read.lock() = 0;

                    PlayerStatus::Playing
                }
                PlayerStatus::Playing => PlayerStatus::Paused,
                _ => PlayerStatus::Playing,
            };

            continue;
        }

        let curstat = *status.lock();

        if curstat == PlayerStatus::Playing {
            let block = {
                let mut x = cache_line.lock();
                let block = x.front().cloned();

                if block.is_some() {
                    x.pop_front();
                }

                block
            };

            if let Some(a) = block {
                audio.write(&a);

                bytes_played += a.len();
            } else if *bytes_read.lock() >= filesize {
                qemu_note!("No block!");

                *status.lock() = PlayerStatus::Stop;
            }
        }

        let st = timestamp();

        draw_ui(
            &mut canvas,
            &curstat,
            &filepath,
            &meta,
            &cache_line.lock().iter().map(|a| a.len()).sum(),
            &bytes_to_seconds(&fmtdata, bytes_played),
            &total_time_seconds,
        );

        render_canvas(&mut canvas);

        qemu_note!("Rendered in: {} ms", timestamp() - st);
    }

    audio.close();

    *is_running.lock() = false;

    Ok(())
}
