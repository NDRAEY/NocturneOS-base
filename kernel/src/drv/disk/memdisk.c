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

#include "generated/diskman.h"
#include "generated/diskman_commands.h"

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
                            SAYORI_UNUSED const uint8_t *parameters,
                            SAYORI_UNUSED uintptr_t param_len,
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

bool memdisk_create(const char* preffered_id, void* memory, size_t size) {
    if(size == 0) {
        return false;
    }

    if(memory == NULL) {
        memory = kcalloc(size, sizeof(size_t));

        if(memory == NULL) {
            return false;
        }
    }

    if(preffered_id == NULL) {
        preffered_id = diskman_generate_new_id("mem");
    }

    memdisk_t* disk = allocate_one(memdisk_t);
    disk->memory = memory;
    disk->size = size;
    
    char* new_id = diskman_generate_new_id(preffered_id);

    diskman_register_drive(
        "Memory disk",
        new_id,
        disk,
        memdisk_diskman_read,
        memdisk_diskman_write,
        memdisk_diskman_control
    );

    qemu_note("Memory: %p; Size: %x; Disk: `%s`", memory, size, preffered_id);

    return true;
}
