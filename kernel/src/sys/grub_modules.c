//
// Created by ndraey on 2/10/24.
//

#include "sys/grub_modules.h"
#include "multiboot.h"
#include "io/ports.h"
#include "lib/string.h"
#include "drv/disk/initrd.h"

size_t grub_last_module_end = 0;

// Needed to configure physical memory manager
void grub_modules_prescan(const multiboot_header_t* hdr) {
    const multiboot_module_t *mod = ((const multiboot_module_t *)hdr->mods_addr) + (hdr->mods_count - 1);

    grub_last_module_end = mod->mod_end;

    qemu_note("Set end to: %x", grub_last_module_end);
}

extern volatile size_t NOCTURNE_ksym_data_start;
extern volatile size_t NOCTURNE_ksym_data_end;

void grub_modules_init(const multiboot_header_t* hdr) {
    qemu_log("Initializing kernel modules...");

    if(hdr->mods_count == 0) {
        qemu_err("No modules were connected!");
        return;
    }

    qemu_log("Found %d modules", hdr->mods_count);

    multiboot_module_t* module_list = (multiboot_module_t*)hdr->mods_addr;

    qemu_log("Module list at: %x", module_list);

    for (size_t i = 0; i < hdr->mods_count; i++) {
        multiboot_module_t *mod = module_list + i;

        // size_t mod_size = mod->mod_end - mod->mod_start;

        qemu_log("[Module] Found module #%d. (Start: %x | End: %x | Size: %d); CMD: %s (at %x)",
                 i,
                 mod->mod_start,
                 mod->mod_end,
                 mod->mod_end - mod->mod_start,
                 (char*)mod->cmdline,
                 (size_t)mod->cmdline
        );

        if (strcmp((const char *) mod->cmdline, "initrd_tarfs") == 0) {
            initrd_tarfs(mod->mod_start, mod->mod_end);
        }
        
        if (strcmp((const char *) mod->cmdline, "ksym") == 0) {
            qemu_ok("The kernel symbol table is present!");

            NOCTURNE_ksym_data_start = mod->mod_start;
            NOCTURNE_ksym_data_end = mod->mod_end;
        }
    }
}