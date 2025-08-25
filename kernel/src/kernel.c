/**
 * @file kernel.c
 * @author Пиминов Никита (nikita.piminoff@yandex.ru), NDRAEY >_ (pikachu_andrey@vk.com)
 * @brief Основная точка входа в ядро
 * @version 0.4.1
 * @date 2022-11-01
 * @copyright Copyright SayoriOS Team (c) 2022-2025
 */

#include "kernel.h"

#include <drv/fpu.h>
#include <sys/unwind.h>

#include "io/ports.h"
#include "io/tty.h"
#include "mem/pmm.h"
#include "mem/vmm.h"
#include "drv/audio/ac97.h"
#include "sys/mtrr.h"
#include "net/ipv4.h"

#include "net/stack.h"
#include "drv/audio/hda.h"
#include "sys/grub_modules.h"
#include "sys/file_descriptors.h"
#include "drv/ps2.h"
#include "net/dhcp.h"
#include "gfx/intel.h"

#include <drv/disk/media_notifier.h>

#include <lib/pixel.h>
#include <net/socket.h>

#include <sys/sse.h>
#include <user/env.h>
#include <arch/init.h>

size_t VERSION_MAJOR = 0;    /// Версия ядра
size_t VERSION_MINOR = 4;    /// Пре-релиз
size_t VERSION_PATCH = 1;    /// Патч

char* OS_ARCH = "i386";      /// Архитектура
char* VERSION_NAME = "Sea";  /// Имя версии (изменяется вместе с минорной части версии)

extern bool ps2_channel2_okay;

uint32_t init_esp = 0;
bool test_pcs = true;
bool test_network = true;
bool initRD = false;
size_t kernel_start_time = 0;

/**
 * @brief Обработка команд указаных ядру при загрузке
 *
 * @param cmd - Команды
 */

void kHandlerCMD(char *cmd)
{
    qemu_log("Kernel command line at address %x and contains: '%s'", (size_t)cmd, cmd);

    if (strlen(cmd) == 0)
        return;

    size_t kCMDc = str_cdsp(cmd, " ");
    uint32_t kCMDc_c = 0;
    char *out[128] = {0};
    str_split(cmd, out, " ");
    for (size_t i = 0; kCMDc >= i; i++)
    {
        kCMDc_c = str_cdsp(out[i], "=");
        char *out_data[128] = {0};
        if (kCMDc_c != 1)
        {
            qemu_log("[kCMD] [%d] %s is ignore.", i, out[i]);
            continue;
        }
        str_split(out[i], out_data, "=");
        if (strcmpn(out_data[0], "bootscreen"))
        {
            // Config BOOTSCREEN
            if (strcmpn(out_data[1], "minimal"))
            {
                bootScreenChangeMode(1);
            }
            else if (strcmpn(out_data[1], "light"))
            {
                bootScreenChangeTheme(1);
            }
            else if (strcmpn(out_data[1], "dark"))
            {
                bootScreenChangeTheme(0);
            }
            else
            {
                qemu_log("\t Sorry, no support bootscreen mode!");
            }
        }
        if (strcmpn(out_data[0], "disable"))
        {
            if (strcmpn(out_data[1], "coms"))
            {
                __com_setInit(1, 0);
                __com_setInit(2, 0);
                __com_setInit(3, 0);
                __com_setInit(4, 0);
                qemu_log("\t COM-OUT DISABLED");
            }
            else if (strcmpn(out_data[1], "network"))
            {
                test_network = false;
                qemu_log("\t NETWORK DISABLED");
            }
            else if (strcmpn(out_data[1], "pc-speaker"))
            {
                test_pcs = false;
                qemu_log("\t PC-Speaker DISABLED");
            }
            else
            {
                qemu_log("\t Sorry, no support!");
            }
        }
    }
}

/**
 * @brief Точка входа в ядро
 *
 * @param multiboot_header_t mboot - Информация MultiBoot
 * @param initial_esp -  Точка входа
 */

extern size_t CODE_start;
extern size_t CODE_end;
extern size_t DATA_start;
extern size_t DATA_end;
extern size_t RODATA_start;
extern size_t RODATA_end;
extern size_t BSS_start;
extern size_t BSS_end;

extern void rust_main();
extern void keyboard_buffer_init();
extern void ipc_init();

extern void fpu_save();

void new_nsh();

extern size_t KERNEL_BASE_pos;
extern size_t KERNEL_END_pos;

void scan_kernel(const multiboot_header_t *mboot) {
    kernel_start = (size_t)&KERNEL_BASE_pos;
	kernel_end = (size_t)&KERNEL_END_pos;

    qemu_log("Flags: %x", mboot->flags);
    qemu_log("MemLower: %x", mboot->mem_lower);
    qemu_log("MemUpper: %x", mboot->mem_upper);
    qemu_log("BootDevice: %x", mboot->boot_device);
    qemu_log("CmdLine: %x", mboot->cmdline);
    qemu_log("ModsCount: %x", mboot->mods_count);
    qemu_log("ModsAddr: %x", mboot->mods_addr);
    qemu_log("Num: %x", mboot->num);
    qemu_log("Size: %x", mboot->size);
    qemu_log("Addr: %x", mboot->addr);
    qemu_log("Shndx: %x", mboot->shndx);
    qemu_log("MmapLength: %x", mboot->mmap_length);
    qemu_log("MmapAddr: %x", mboot->mmap_addr);

	qemu_log("Kernel: %x - %x", kernel_start, kernel_end);

    multiboot_module_t* module_list = (multiboot_module_t*)mboot->mods_addr;
    size_t count = mboot->mods_count;

    for (size_t i = 0; i < count; i++) {
        const multiboot_module_t *mod = module_list + i;
        (void)mod;

        qemu_log("Module #%d: Start: %x; End: %x; Size: %d k",
                i,
                mod->mod_start,
                mod->mod_end,
                (mod->mod_end - mod->mod_start) >> 10
        );
    }

#ifndef RELEASE
    const multiboot_module_t* last_module = module_list + count - 1;
    qemu_log("Bitmap: %x - %x", last_module->mod_end, last_module->mod_end + PAGE_BITMAP_SIZE);
#endif
}

/*
  Спаси да сохрани этот кусок кода
  Да на все твое кодерская воля
  Да прибудет с тобой, священный код
  Я тебя благославляю
*/
void __attribute__((noreturn)) kmain(const multiboot_header_t *mboot, uint32_t initial_esp)
{
    __asm__ volatile("movl %%esp, %0" : "=r"(init_esp));
    
    arch_init();

    __com_setInit(1, 1);
    __com_init(PORT_COM1);
    
    scan_kernel(mboot);

    framebuffer_addr = (uint8_t *)(size_t)(mboot->framebuffer_addr);

    drawASCIILogo(0);

    qemu_log("SayoriOS v%d.%d.%d\nBuilt: %s",
             VERSION_MAJOR, VERSION_MINOR, VERSION_PATCH, // Версия ядра
             __TIMESTAMP__                                // Время окончания компиляции ядра
    );

    qemu_log("Bootloader header at: %x", (size_t)mboot);

    qemu_log("SSE: %s", sse_check() ? "Supported" : "Not supported");

    qemu_log("Setting `Interrupt Descriptor Table`...");
    init_descriptor_tables();
    
    qemu_log("Setting `ISR`...");
    isr_init();

    qemu_log("Initializing FPU...");
    fpu_init();

    init_timer(CLOCK_FREQ);

    __asm__ volatile("sti");

    qemu_log("Checking RAM...");
    check_memory_map((memory_map_entry_t *)mboot->mmap_addr, mboot->mmap_length);
    qemu_log("Memory summary:");
    qemu_log("    Code: %x - %x", (size_t)&CODE_start, (size_t)&CODE_end);
    qemu_log("    Data: %x - %x", (size_t)&DATA_start, (size_t)&DATA_end);
    qemu_log("    Read-only data: %x - %x", (size_t)&RODATA_start, (size_t)&RODATA_end);
    qemu_log("    BSS: %x - %x", (size_t)&BSS_start, (size_t)&BSS_end);
    qemu_log("Memory manager initialization...");

    grub_modules_prescan(mboot);

    init_paging();

    mark_reserved_memory_as_used((memory_map_entry_t *)mboot->mmap_addr, mboot->mmap_length);

    qemu_ok("PMM Ok!");

    vmm_init();
    qemu_ok("VMM OK!");
    
    init_syscalls();
    
    switch_qemu_logging();
    
    kHandlerCMD((char *)mboot->cmdline);
    
    qemu_log("Initializing Task Manager...");
    init_task_manager();

    ipc_init();

    // drv_vbe_init(mboot);

    qemu_log("Audio system init");
    audio_system_init();

    // audio_system_add_output("Duper", NULL, noc_open, noc_set_volume, noc_set_rate, noc_write, noc_close);

    // audio_system_open(0);

    qemu_log("FSM Init");
    fsm_init();

    qemu_log("Registration of file system drivers...");
    fs_tarfs_register();    
    fs_fatfs_init();  
    fs_iso9660_init();
    fs_noctfs_init();
    
    grub_modules_init(mboot);

    // TarFS registered by grub_modules_init will always have the name `rd0`.
    fsm_dpm_update("rd0");

    kernel_start_time = getTicks();

    mtrr_init();

    qemu_log("Initializing the virtual video memory manager...");
    init_vbe(mboot);

    psf_init("rd0:/Sayori/Fonts/UniCyrX-ibm-8x16.psf");

    qemu_log("Initalizing fonts...");
    tty_init();

    clean_screen();

    // tty_puts("ABCDEFGHIJKLMNOPQRSTUVWXYZ\n");
    // tty_puts("abcdefghijklmnopqrstuvwxyz\n");
    // tty_puts("0123456789\n");
    // tty_puts("!@#$%^&*()\n");
    // tty_puts("`~\n");
    // tty_puts("АБВГДЕЁЖЗИЙКЛМНОПРСТУФХЦЧШЩЪЫЬЭЮЯ\n");
    // tty_puts("абвгдеёжзийклмнопрстуфхцчшщъыьэюя\n");

    // while(1)
    //     ;

    bootScreenInit(15);
    bootScreenLazy(true);

    keyboard_buffer_init();

    ps2_init();

    ps2_keyboard_init();

    if (ps2_channel2_okay)
    {
        mouse_install();
    }

    ps2_keyboard_install_irq();
    ps2_mouse_install_irq();

    bootScreenPaint("PCI Setup...");
    pci_scan_everything();

    bootScreenPaint("Инициализация ATA...");
    ata_init();
    ata_dma_init();
    
    bootScreenPaint("Калибровка датчика температуры процессора...");
    cputemp_calibrate();

    file_descriptors_init();

    configure_env();

    netcards_list_init();

    bootScreenPaint("Инициализация сетевого стека...");
    netstack_init();

    bootScreenPaint("Инициализация ARP...");
    arp_init();

    bootScreenPaint("Инициализация RTL8139...");
    rtl8139_init();

    bootScreenPaint("Инициализация DHCP...");
    dhcp_init_all_cards();

    bootScreenPaint("Готово...");
    bootScreenClose(0x000000, 0xFFFFFF);
    tty_set_bgcolor(COLOR_BG);

    tty_printf("NocturneOS v%d.%d.%d '%s'\nДата компиляции: %s\n",
               VERSION_MAJOR, VERSION_MINOR, VERSION_PATCH, // Версия ядра
               VERSION_NAME,
               __TIMESTAMP__                                // Время окончания компиляции ядра
    );

    tty_printf("\nВлюбиться можно в красоту, но полюбить - лишь только душу.\n(c) Уильям Шекспир\n");

    ahci_init();

    sayori_time_t time = get_time();
    tty_printf("\nВремя: %02d:%02d:%02d\n", time.hours, time.minutes, time.seconds);

    tty_printf("Listing ATA disks:\n");

    ata_list();

    tty_taskInit();

    {
        RSDPDescriptor *rsdp = rsdp_find();
        qemu_log("RSDP at: %p", rsdp);

        if (rsdp)
        {
            acpi_scan_all_tables(rsdp->RSDTaddress);

            find_facp(rsdp->RSDTaddress);
        }
        else
        {
            tty_printf("ACPI not supported! (Are you running in UEFI mode?)\n");
            qemu_err("ACPI not supported! (Are you running in UEFI mode?)");
        }
    }

    //tty_printf("Processors: %d\n", system_processors_found);

    if (test_network)
    {
        tty_printf("Listing network cards:\n");

        uint8_t mac_buffer[6] = {0};

        for (size_t i = 0; i < netcards_get_count(); i++)
        {
            netcard_entry_t *entry = netcard_get(i);

            tty_printf("\tName: %s\n", entry->name);
            entry->get_mac_addr(mac_buffer);

            tty_printf("\tMAC address: %x:%x:%x:%x:%x:%x\n",
                        mac_buffer[0],
                        mac_buffer[1],
                        mac_buffer[2],
                        mac_buffer[3],
                        mac_buffer[4],
                        mac_buffer[5]);
        }
    }

    ac97_init();
    // hda_init();

    /// Обновим данные обо всех дисках
    fsm_dpm_update(NULL);

    igfx_init();

    rust_main();

    qemu_log("System initialized everything at: %f seconds.", (double)(getTicks() - kernel_start_time) / getFrequency());
    tty_printf("System initialized everything at: %.2f seconds.\n", (double)(getTicks() - kernel_start_time) / getFrequency());

    launch_media_notifier();

    // net_test();

    new_nsh();

    while (1)
        ;
}

// TODO: The following code is a sketch of future socket system. It may change, may not.

// void net_test() {
//     socket_address_t server_addr = {
//         .address = {192, 168, 1, 128},
//         .port = 9999
//     };

//     char data[32] = {0};

//     socket_t* srv_sock = socket_new(&server_addr, PROTO_TCP);
//     socket_t* client_sock = socket_listen(srv_sock);

//     if(client_sock) {
//         socket_read_until_newline(client_sock, data);

//         socket_close(client_sock);
//     }

//     int length = strlen(data);

//     qemu_note("Received %d bytes with data: `%s`", length, data);
//     socket_close(srv_sock);
// }

void sysidle() {
    while(1) {
        __asm__ volatile("hlt");
      yield();
    }
}
