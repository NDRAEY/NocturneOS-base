#include "drv/disk/memdisk.h"

#include <lib/math.h>

#include "drv/pci.h"
#include "io/ports.h"
#include "io/tty.h"
#include "mem/pmm.h"
#include "mem/vmm.h"
#include "sys/isr.h"
#include "drv/disk/ata.h"
#include "drv/atapi.h"
#include "net/endianess.h"
#include "drv/disk/dpm.h"

size_t memdisk_dpm_read(size_t disk, uint64_t high_offset, uint64_t low_offset, size_t size, void* Buffer){
	memdisk_t* memdisk = dpm_info(disk + 65).Point;

    size_t end_offset = low_offset + size;

    if(end_offset > memdisk->size) {
        size = memdisk->size - low_offset;
    }

    memcpy(Buffer, (char*)memdisk->memory + low_offset, size);
    
    return size;
}

size_t memdisk_dpm_write(size_t disk, uint64_t high_offset, uint64_t low_offset, size_t size, void* Buffer){
	memdisk_t* memdisk = dpm_info(disk + 65).Point;

    size_t end_offset = low_offset + size;

    if(end_offset > memdisk->size) {
        size = memdisk->size - low_offset;
    }

    memcpy(memdisk->memory + low_offset, Buffer, size);

    return size;
}

bool memdisk_create(char letter, void* memory, size_t size) {
    if(size == 0) {
        return false;
    }

    if(letter == 0) {
        letter = (char)dpm_searchFreeIndex(0);
    }

    if(memory == 0) {
        memory = kcalloc(size, 1);
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

    qemu_note("Memory: %p; Size: %x; Letter: %c; Index: %d", memory, size, letter, disk_inx);

    if (disk_inx < 0){
        qemu_err("[MEMDISK] [ERROR] An error occurred during disk registration, error code: %d", disk_inx);
        
        return false;
    } else {
        qemu_ok("[MEMDISK] [Successful] Registering OK");
        dpm_fnc_write(letter, &memdisk_dpm_read, &memdisk_dpm_write);

        return true;
    }
}
