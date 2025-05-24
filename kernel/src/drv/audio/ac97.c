// AC'97 driver by NDRAEY

// FIXME: Not working in VirtualBox, only in QEMU

#include <common.h>
#include <lib/math.h>
#include <drv/audio/ac97.h>
#include "io/ports.h"
#include "generated/pci.h"
#include "mem/pmm.h"
#include "lib/stdio.h"
#include "mem/vmm.h"
#include "sys/scheduler.h"
#include "generated/audiosystem_headers.h"

uint8_t ac97_busnum, ac97_slot, ac97_func;

bool ac97_initialized = false;
bool ac97_varibale_sample_rate = false;

uint16_t native_audio_mixer = 0;
uint16_t native_audio_bus_master = 0;

uint8_t ac97_bar0_type = 0;
uint8_t ac97_bar1_type = 0;

size_t  currently_pages_count = 0;

uint32_t current_sample_rate = 44100;

// The kernel already mapped this 1:1, so no conversion is needed.
__attribute__((aligned(PAGE_SIZE))) volatile AC97_BDL_t ac97_buffer[32];

char* ac97_audio_buffer = 0;
// size_t ac97_audio_buffer_phys;

volatile size_t ac97_lvi = 0;

#define AUDIO_BUFFER_SIZE (128 * KB)

// Volume in dB, not % (max 64)
void ac97_set_master_volume(uint8_t left, uint8_t right, bool mute) {
    const uint16_t value = ((right & 63) << 0)
        | ((left & 63) << 8)
        | ((uint16_t)mute << 15);
    outw(native_audio_mixer + NAM_SET_MASTER_VOLUME, value);
}

// Volume in dB, not % (max 32)
void ac97_set_pcm_volume(uint8_t right, uint8_t left, bool mute) {
    uint16_t value = ((right & 31) << 0)
        | ((left & 31) << 8)
        | ((uint16_t)mute << 15);
    outw(native_audio_mixer + NAM_SET_PCM_VOLUME, value);
}

void ac97_set_pcm_sample_rate(uint16_t sample_rate) {
    if(!ac97_varibale_sample_rate
	|| sample_rate > AC97_MAX_RATE
	|| sample_rate < AC97_MIN_RATE)
		return;
    
    outw(native_audio_mixer + NAM_SAMPLE_RATE, sample_rate);
}

void ac97_reset_channel() {
    uint8_t input = inb(native_audio_bus_master + 0x1b);
    // outb(native_audio_bus_master + 0x1b, 0x02);
    outb(native_audio_bus_master + 0x1b, input | 0x02);

    while(inb(native_audio_bus_master + 0x1b) & 0x02) {
        yield();
    }
}

void ac97_clear_status_register() {
    outb(native_audio_bus_master + 0x16, 0x1C);
}

void _ac97_update_bdl(uint32_t address) {
    outl(native_audio_bus_master + NABM_PCM_OUT, address);  // BDL Address register
}

void ac97_update_bdl() {
    _ac97_update_bdl((uint32_t) ac97_buffer);
}

void ac97_update_lvi(uint8_t index) {
    outb(native_audio_bus_master + NABM_PCM_OUT + 0x05, index & 0b11111);
}

void ac97_set_play_sound(bool play) {
    uint8_t input = inb(native_audio_bus_master + 0x1B);

    if(play) {
        input |= 0x1;
    } else {
        input &= ~0x1;
    }

    outb(native_audio_bus_master + 0x1b, input);
}

void ac97_init() {
    // Find device
    uint8_t result = pci_find_device(AC97_VENDOR, AC97_DEVICE, &ac97_busnum, &ac97_slot, &ac97_func);

    if(!result) {
        qemu_err("AC'97 not connected!");
        return;
    } else {
        qemu_ok("Detected AC'97");
    }

    // Enable IO Busmastering

    pci_enable_bus_mastering(ac97_busnum, ac97_slot, ac97_func);

    // Get NAM and NABM adresses for port i/o.

    native_audio_mixer = pci_read32(ac97_busnum, ac97_slot, ac97_func, 0x10) & 0xffff;  // BAR0
    native_audio_bus_master = pci_read32(ac97_busnum, ac97_slot, ac97_func, 0x14) & 0xffff; // BAR1

    native_audio_mixer &= ~0xf;
    native_audio_bus_master &= ~0xf;

    qemu_log("NAM: %x; NABM: %x", native_audio_mixer, native_audio_bus_master);

    const uint16_t extended_id = inw(native_audio_mixer + NAM_EXTENDED_ID);

    #ifndef RELEASE
    const size_t rev = (extended_id >> 10) & 0b11;
    const char* rev_strs[] = {"r < 21", "r22", "r23"};
    qemu_log("Codec revision: (%d) %s", rev, rev_strs[rev]);
    #endif

    uint32_t gc = inl(native_audio_bus_master + NABM_GLOBAL_CONTROL);
    qemu_log("Received global control: (%d) %x", gc, gc);
    gc |= 1 << 1;  // Cold reset
    outl(native_audio_bus_master + NABM_GLOBAL_CONTROL, gc);
    outw(native_audio_mixer + NAM_RESET, 1);
    qemu_log("Cold reset");

    #ifndef RELEASE
    const uint32_t raw_status = inl(native_audio_bus_master + NABM_GLOBAL_STATUS);

    qemu_log("Status: %d (%x)", raw_status, raw_status);

	const ac97_global_status_t status = *(ac97_global_status_t*)(&status);
    qemu_log("Status Reserved: %d", status.reserved);
    qemu_log("Status Channels: %d", (status.channel==0 ? 2 : (status.channel==1 ? 4 : (status.channel==2 ? 6 : 0))));
    qemu_log("Status Samples: %s", status.sample==1?"16 and 20 bits":"only 16 bits");
    #endif
    
    uint16_t extended_audio = inw(native_audio_mixer + NAM_EXTENDED_AUDIO);
    qemu_log("Status: %d", extended_audio);

    if((extended_id & 1) != 0) { // Check for variable sample rate
        qemu_log("AC'97 supports variable sample rate!!!");
        extended_audio |= 1;
        ac97_varibale_sample_rate = true;
    }

    outw(native_audio_mixer + NAM_EXTENDED_AUDIO, extended_audio);

    ac97_set_pcm_sample_rate(ac97_varibale_sample_rate ? 44100 : 48000);

    ac97_set_master_volume(0, 0, false);
    ac97_set_pcm_volume(0, 0, false);

    ac97_audio_buffer = kmalloc_common(AUDIO_BUFFER_SIZE, PAGE_SIZE);
    // ac97_audio_buffer = kmalloc_common_contiguous(get_kernel_page_directory(), ALIGN(AUDIO_BUFFER_SIZE, PAGE_SIZE) / PAGE_SIZE);
    memset(ac97_audio_buffer, 0, AUDIO_BUFFER_SIZE);
    // ac97_audio_buffer_phys = virt2phys(get_kernel_page_directory(),
    //                                    (virtual_addr_t)ac97_audio_buffer);

    qemu_log("Updated capabilities");

    ac97_FillBDLs();

    ac97_initialized = true;

    audio_system_add_output("AC'97", NULL, ac97as_open, ac97as_set_volume, ac97as_set_rate, ac97as_write, ac97as_close);

    qemu_log("AC'97 initialized successfully!");
}

bool ac97_is_initialized() {
    return ac97_initialized;
}

void ac97_FillBDLs() {
    size_t sample_divisor = 2;  // AC'97 is 2-channel, so one sample is two bytes long.

    // We need to fill ALL BDL entries.
    // If we don't do that, we can encounter lags and freezes, because DMA doesn't stop
    // even when its read pointer reached the end marker. (It will scroll to the end)
    // So we need to spread buffer on BDL array

    size_t bdl_span = AUDIO_BUFFER_SIZE / 32; // It's the size of each transfer (32 is the count of BDLs)

    size_t filled = 0;
    for (size_t j = 0; j < AUDIO_BUFFER_SIZE; j += bdl_span) {
        size_t phys = virt2phys_precise(get_kernel_page_directory(), (virtual_addr_t)(ac97_audio_buffer + j));

        ac97_buffer[filled].memory_pos = phys;
        ac97_buffer[filled].sample_count = bdl_span / sample_divisor;

        qemu_log("[%d] %x -> %x", filled, ac97_buffer[filled].memory_pos, ac97_buffer[filled].sample_count * sample_divisor);

        filled++;
    }

    // qemu_printf("Fills: %d\n", filled);
    
    ac97_lvi = 31;

    ac97_buffer[ac97_lvi].flags = (1 << 14) | (1 << 15);

    ac97_update_bdl();
    ac97_update_lvi(ac97_lvi);
}

void ac97_WriteAll(const void* buffer, size_t size) {
    qemu_log("Start");

    size_t loaded = 0;
    // ac97_reset_channel();

    while(loaded < size) {
        size_t block_size = MIN(size - loaded, AUDIO_BUFFER_SIZE);

        memcpy(ac97_audio_buffer,
                (char*)buffer + loaded,
                block_size);

        qemu_printf("[%d] memcpy: ac97 buffer: %x; original buffer: %x; size: %d\n",
            timestamp(),
            ac97_audio_buffer,
            (char*)buffer + loaded,
            block_size);

        // Zero out all the remaining space
        if (block_size < AUDIO_BUFFER_SIZE) {
            memset(ac97_audio_buffer + block_size, 0, AUDIO_BUFFER_SIZE - block_size);
        }

        ac97_update_lvi(ac97_lvi);
        ac97_set_play_sound(true);
        // ac97_clear_status_register();

        // Poll while playing.
        while (!(inb(native_audio_bus_master + 0x16) & (1 << 1))) {
            __asm__ volatile("nop");
            yield();
        }

        loaded += block_size;
    }

    qemu_log("Finish");
}

void ac97as_open(void* priv) {
    (void)priv;
    qemu_log("OPEN!\n");
}

// Volume is in range 0 - 100
void ac97as_set_volume(void* priv, uint8_t left, uint8_t right) {
    (void)priv;
    qemu_log("SET VOLUME: %d %d!\n", left, right);

    size_t lch_mas = ((100 - left) * 63) / 100;
    size_t rch_mas = ((100 - right) * 63) / 100;
    size_t lch_pcm = ((100 - left) * 31) / 100;
    size_t rch_pcm = ((100 - right) * 31) / 100;

    qemu_log("Converted to: %d %d %d %d!\n", lch_mas, rch_mas, lch_pcm, rch_pcm);

    ac97_set_master_volume(lch_mas, rch_mas, false);
	ac97_set_pcm_volume(lch_pcm, rch_pcm, false);
}

void ac97as_set_rate(void* priv, uint32_t rate) {
    (void)priv;
	ac97_set_pcm_sample_rate(rate);
    qemu_log("RATE: %dHz!\n", rate);
}

void ac97as_write(void* priv, const uint8_t* data, size_t len) {
    (void)priv;
    qemu_log("WRITE: %p <%d>!\n", data, len);
    ac97_WriteAll(data, len);
}

void ac97as_close(void* priv) {
    (void)priv;
    qemu_log("CLOSE!\n");

    ac97_clear_status_register();
}
