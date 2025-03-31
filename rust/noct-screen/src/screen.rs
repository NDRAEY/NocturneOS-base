#[derive(Debug)]
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
            PixelFormat::RGB => 3,
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

        let mut color = ((r as u32) << ofs.r)
            | ((g as u32) << ofs.g)
            | ((b as u32) << ofs.b);

        if let Some(alpha) = ofs.a {
            color |= a.map(|x| (x as u32) << alpha).unwrap_or(0xff_000000);
        }

        color
    }
}

#[derive(Debug)]
pub struct Resolution {
    pub width: usize,
    pub height: usize,
}

pub struct Screen<'fb> {
    framebuffer: &'fb mut [u8],
    resolution: Resolution,
    pixel_format: PixelFormat,
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
                core::slice::from_raw_parts_mut(
                    framebuffer,
                    width * height * pixel_format.bits_per_pixel(),
                )
            },
            resolution: Resolution { width, height },
            pixel_format,
        }
    }

    #[inline]
    pub fn stride(&self) -> usize {
        self.resolution.width * self.resolution.height * self.pixel_format.bits_per_pixel()
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
        (x * (self.pixel_format().bits_per_pixel() >> 3)) + y * self.stride()
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
        
        let (r, g, b, a) = (pixels[offs.r >> 3], pixels[offs.g >> 3], pixels[offs.b >> 3], None);

        Some(self.pixel_format().to_universal(r, g, b, a))
    }
}
