#include "drv/disk/memdisk.h"

#include <lib/math.h>

#include "io/ports.h"
#include "io/tty.h"
#include "mem/pmm.h"
#include "mem/vmm.h"
#include "sys/isr.h"
#include "drv/disk/ata.h"
#include "drv/atapi.h"
#include "net/endianess.h"

#include "drv/disk/dpm.h"

#include "generated/diskman.h"
#include "generated/diskman_commands.h"

size_t memdisk_dpm_read(size_t disk, uint64_t high_offset, uint64_t low_offset, size_t size, void* Buffer){
    (void)high_offset;
    // qemu_printf("RD => D: %d; LO: (%x, %x); SZ: %d; B: %x\n", disk, low_offset, size, Buffer);

    memdisk_t* memdisk = dpm_info(disk + 65).Point;

    size_t end_offset = low_offset + size;

    if(end_offset > memdisk->size) {
        size = memdisk->size - low_offset;
    }

    memcpy(Buffer, (char*)memdisk->memory + low_offset, size);
    
    return size;
}

size_t memdisk_dpm_write(size_t disk, uint64_t high_offset, uint64_t low_offset, size_t size, const void* Buffer){
    (void)high_offset;

    qemu_printf("WRITE? => D: %d; LO: (%x, %x); SZ: %d; B: %x\n", disk, low_offset, size, Buffer);

    while(1)
    ;

	memdisk_t* memdisk = dpm_info(disk + 65).Point;

    size_t end_offset = low_offset + size;

    if(end_offset > memdisk->size) {
        size = memdisk->size - low_offset;
    }

    memcpy(memdisk->memory + low_offset, Buffer, size);

    return size;
}

static int64_t memdisk_diskman_read(void* priv_data, uint64_t location, uint64_t size, uint8_t* buf) {
    memdisk_t* memdisk = (memdisk_t*)priv_data;
    size_t end_offset = (size_t)location + size;

    if(end_offset > memdisk->size) {
        size = memdisk->size - (size_t)location;
    }

    memcpy(buf, (char*)memdisk->memory + (size_t)location, size);
    
    return size;
}

static int64_t memdisk_diskman_write(void* priv_data, uint64_t location, uint64_t size, const uint8_t* buf) {
    qemu_printf("Attempted writing to memdisk!\n");

    while(1)
    ;

	memdisk_t* memdisk = (memdisk_t*)priv_data;

    size_t end_offset = (size_t)location + size;

    if(end_offset > memdisk->size) {
        size = memdisk->size - (size_t)location;
    }

    memcpy(memdisk->memory + (size_t)location, buf, size);

    return size;
}

static int64_t memdisk_diskman_control(void *priv_data,
                            uint32_t command,
                            const uint8_t *parameters,
                            uintptr_t param_len,
                            uint8_t *buffer,
                            uintptr_t buffer_len) {
	memdisk_t* memdisk = (memdisk_t*)priv_data;

    if(command == DISKMAN_COMMAND_GET_DRIVE_TYPE) {
        if(buffer == NULL || buffer_len < 4) {
            return -1;
        }

        // *(uint32_t*)buffer = 0;
        // 0 is HardDisk
        memset(buffer, 0, 4);
    } else if(command == DISKMAN_COMMAND_GET_MEDIUM_CAPACITY) {
        if(buffer == NULL || buffer_len < 12) {
            return -1;
        }

        *(uint64_t*)buffer = memdisk->size;
        *(uint32_t*)(buffer + 8) = 1;
    } else if(command == DISKMAN_COMMAND_GET_MEDIUM_STATUS) {
        if(buffer == NULL || buffer_len < 4) {
            return -1;
        }

        // 2 - Online
        *(uint32_t*)(buffer) = 2;
    }

    return -1;
}

bool memdisk_create(char letter, void* memory, size_t size) {
    if(size == 0) {
        return false;
    }

    if(letter == 0) {
        letter = (char)dpm_searchFreeIndex(0);
    }

    if(memory == 0) {
        memory = kcalloc(size, sizeof(size_t));
    }

    memdisk_t* disk = allocate_one(memdisk_t);
    disk->memory = memory;
    disk->size = size;

    int disk_inx = dpm_reg(
            letter,
            "Memory disk",
            "None",
            1,
            size,
            size,
            1,
            3,
            "DISK1234567890",
            disk
    );

    
    char* new_id = diskman_generate_new_id("mem");

    diskman_register_drive(
        "Memory disk",
        new_id,
        disk,
        memdisk_diskman_read,
        memdisk_diskman_write,
        memdisk_diskman_control
    );

    qemu_note("Memory: %p; Size: %x; Letter: %c; Index: %d", memory, size, letter, disk_inx);

    if (disk_inx < 0) {
        qemu_err("[MEMDISK] [ERROR] An error occurred during disk registration, error code: %d", disk_inx);
        
        return false;
    } else {
        // dpm_fnc_write(letter, &memdisk_dpm_read, &memdisk_dpm_write);
        dpm_set_read_func(letter, &memdisk_dpm_read);
        dpm_set_write_func(letter, &memdisk_dpm_write);
        qemu_ok("[MEMDISK] [Successful] Registering OK");

        return true;
    }
}
