use core::{fmt::Debug, slice::from_raw_parts_mut};

use num_derive::FromPrimitive;

#[derive(Clone, Debug, FromPrimitive)]
pub enum PixelFormat {
    RGB,
    // ...
}

pub struct PixelFormatOffsets {
    r: usize,
    g: usize,
    b: usize,
    a: Option<usize>,
}

impl PixelFormat {
    pub fn bits_per_pixel(&self) -> usize {
        match self {
            PixelFormat::RGB => 24,
            // ...
        }
    }

    pub fn offsets(&self) -> PixelFormatOffsets {
        match self {
            PixelFormat::RGB => PixelFormatOffsets {
                r: 16,
                g: 8,
                b: 0,
                a: None,
            },
        }
    }

    pub fn to_universal(&self, r: u8, g: u8, b: u8, a: Option<u8>) -> u32 {
        let ofs = self.offsets();

        let mut color = ((r as u32) << ofs.r) | ((g as u32) << ofs.g) | ((b as u32) << ofs.b);

        if let Some(alpha) = ofs.a {
            color |= a.map(|x| (x as u32) << alpha).unwrap_or(0xff_000000);
        }

        color
    }
}

#[derive(Clone, Debug)]
pub struct Resolution {
    pub width: usize,
    pub height: usize,
}

pub struct Screen<'a> {
    pub(crate) framebuffer: &'a mut [u8],
    pub(crate) resolution: Resolution,
    pub(crate) pixel_format: PixelFormat,
}

impl Debug for Screen<'_> {
    fn fmt(&self, f: &mut core::fmt::Formatter<'_>) -> core::fmt::Result {
        f.debug_struct("Screen")
            .field("framebuffer", &self.framebuffer.as_ptr().addr())
            .field("resolution", &self.resolution)
            .field("pixel_format", &self.pixel_format)
            .finish()
    }
}

impl<'a> Screen<'a> {
    pub fn new(
        framebuffer: &'a mut [u8],
        width: usize,
        height: usize,
        pixel_format: PixelFormat,
    ) -> Self {
        Self {
            framebuffer,
            resolution: Resolution { width, height },
            pixel_format,
        }
    }

    pub fn from_raw_pointer(
        framebuffer: *mut u8,
        width: usize,
        height: usize,
        pixel_format: PixelFormat,
    ) -> Self {
        Self {
            framebuffer: unsafe {
                from_raw_parts_mut(framebuffer, width * height * pixel_format.bits_per_pixel())
            },
            resolution: Resolution { width, height },
            pixel_format,
        }
    }

    pub fn clone_with_same_reference(&self) -> Self {
        let ptr = self.framebuffer.as_ptr();
        let len = self.framebuffer.len();

        // Here I do the serious crime, now `ln` is a ANOTHER MUTABLE REFERENCE TO THE SAME ADDRESS.
        let ln = unsafe { from_raw_parts_mut(ptr as *mut u8, len) };

        Self {
            framebuffer: ln,
            resolution: self.resolution.clone(),
            pixel_format: self.pixel_format.clone(),
        }
    }

    #[inline]
    pub fn stride(&self) -> usize {
        self.resolution.width * (self.pixel_format.bits_per_pixel() >> 3)
    }

    #[inline]
    pub fn resolution(&self) -> &Resolution {
        &self.resolution
    }

    #[inline]
    pub fn pixel_format(&self) -> &PixelFormat {
        &self.pixel_format
    }

    #[inline]
    fn pixel_offset(&self, x: usize, y: usize) -> usize {
        ((y * self.resolution.width) + x) * (self.pixel_format().bits_per_pixel() >> 3)
    }

    pub fn set_pixel(&mut self, x: usize, y: usize, color: u32) {
        if x >= self.resolution.width || y >= self.resolution.height {
            return;
        }

        let offset = self.pixel_offset(x, y);
        let pixels = &mut self.framebuffer[offset..];

        pixels[0] = (color & 0xff) as u8;
        pixels[1] = ((color >> 8) & 0xff) as u8;
        pixels[2] = ((color >> 16) & 0xff) as u8;
    }

    /// Returns color in 0xAARRGGBB
    pub fn get_pixel(&self, x: usize, y: usize) -> Option<u32> {
        if x >= self.resolution.width || y >= self.resolution.height {
            return None;
        }

        let offset = self.pixel_offset(x, y);
        let pixels = &self.framebuffer[offset..];

        let offs = self.pixel_format().offsets();

        let (r, g, b, a) = (
            pixels[offs.r >> 3],
            pixels[offs.g >> 3],
            pixels[offs.b >> 3],
            None,
        );

        Some(self.pixel_format().to_universal(r, g, b, a))
    }

    pub fn fill(&mut self, color: u32) {
        for y in 0..self.resolution.height {
            for x in 0..self.resolution.width {
                self.set_pixel(x, y, color);
            }
        }
    }

    pub fn framebuffer_raw(&self) -> &[u8] {
        &self.framebuffer
    }

    pub fn framebuffer_raw_mut(&mut self) -> &mut [u8] {
        &mut self.framebuffer
    }
}
