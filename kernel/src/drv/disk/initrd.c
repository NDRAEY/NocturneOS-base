/**
 * @file drv/disk/initrd.c
 * @author Пиминов Никита (nikita.piminoff@yandex.ru)
 * @brief Файл виртуального диска, основаного на основе TarFS
 * @version 0.3.5
 * @date 2023-08-03
 * @copyright Copyright SayoriOS Team (c) 2022-2025
*/

#include <io/ports.h> 
#include <fs/fsm.h>
#include "drv/disk/dpm.h"
#include "drv/disk/memdisk.h"
#include "mem/vmm.h"

int initrd_tarfs(uint32_t start, uint32_t end) {
	qemu_log("[TarFS] Start: %x; End: %x; Size: %d bytes", start, end, end - start);

	size_t initrd_size = end - start;
	if (start > end)
		return 0;

	void* initrd_data = (void*)start;

	qemu_warn("Initrd occupies %d pages", ALIGN(initrd_size, PAGE_SIZE) / PAGE_SIZE);

	// void* mem = kmalloc_common(initrd_size, PAGE_SIZE);

	// memcpy(mem, initrd_data, initrd_size);

	// bool is_ready = memdisk_create('R', mem, initrd_size);
	bool is_ready = memdisk_create('R', initrd_data, initrd_size);
	
	return (int)is_ready;
}

