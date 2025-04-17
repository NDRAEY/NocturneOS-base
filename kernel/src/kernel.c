/**
 * @file kernel.c
 * @author Пиминов Никита (nikita.piminoff@yandex.ru), NDRAEY >_ (pikachu_andrey@vk.com)
 * @brief Основная точка входа в ядро
 * @version 0.3.5
 * @date 2022-11-01
 * @copyright Copyright SayoriOS Team (c) 2022-2025
 */

#include "kernel.h"

#include <drv/fpu.h>
#include <lib/php/explode.h>
#include <sys/unwind.h>

#include "io/ports.h"
#include "mem/pmm.h"
#include "mem/vmm.h"
#include "drv/audio/ac97.h"
#include "sys/mtrr.h"
#include "net/ipv4.h"

#include "net/stack.h"
#include "drv/audio/hda.h"
#include "sys/grub_modules.h"
#include "drv/disk/mbr.h"
#include "sys/file_descriptors.h"
#include "drv/ps2.h"
#include "net/dhcp.h"
#include "gfx/intel.h"

#include <lib/pixel.h>
#include <net/socket.h>

#include <arch/init.h>

size_t VERSION_MAJOR = 0;    /// Версия ядра
size_t VERSION_MINOR = 3;    /// Пре-релиз
size_t VERSION_PATCH = 5;    /// Патч

char* OS_ARCH = "i386";      /// Архитектура
char* VERSION_NAME = "Soul"; /// Имя версии (изменяется вместе с минорной части версии)

#define INITRD_RW_SIZE (1474560) /// Размер виртуального диска 1.44mb floppy

extern bool ps2_channel2_okay;

uint32_t init_esp = 0;
bool test_pcs = true;
bool test_floppy = true;
bool test_network = true;
bool is_rsdp = true;
bool initRD = false;
size_t kernel_start_time = 0;
size_t ramdisk_size = INITRD_RW_SIZE;

void kHandlerCMD(char *);

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

    uint32_t kCMDc = str_cdsp(cmd, " ");
    uint32_t kCMDc_c = 0;
    char *out[128] = {0};
    str_split(cmd, out, " ");
    for (int i = 0; kCMDc >= i; i++)
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
        if (strcmpn(out_data[0], "ramdisk"))
        {
            ramdisk_size = atoi(out_data[1]);
        }
        if (strcmpn(out_data[0], "disable"))
        {
            if (strcmpn(out_data[1], "coms"))
            {
                // FIXME: If uncomment following line of code, it willn't boot
                __com_setInit(1, 0);
                __com_setInit(2, 0);
                __com_setInit(3, 0);
                __com_setInit(4, 0);
                qemu_log("\t COM-OUT DISABLED");
            }
            else if (strcmpn(out_data[1], "floppy"))
            {
                test_floppy = false;
                qemu_log("\t FLOPPY DISABLED");
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
            else if (strcmpn(out_data[1], "rdsp"))
            {
                is_rsdp = false;
                qemu_log("\t RDSP DISABLED");
            }
            else
            {
                qemu_log("\t Sorry, no support!");
            }
        }
        // qemu_log("[kCMD] [%d] %s >\n\tKey: %s\n\tValue:%s",i,out[i],out_data[0],out_data[1]);
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

extern void fs_tarfs_register();
extern void rust_main();
extern void keyboard_buffer_init();
extern void audio_system_init();

void new_nsh();

/*
  Спаси да сохрани этот кусок кода
  Да на все твое кодерская воля
  Да прибудет с тобой, священный код
  Я тебя благославляю
*/
void __attribute__((noreturn)) kmain(multiboot_header_t *mboot, uint32_t initial_esp)
{
    __asm__ volatile("movl %%esp, %0" : "=r"(init_esp));
    
    arch_init();

    __com_setInit(1, 1);
    __com_init(PORT_COM1);

    framebuffer_addr = (uint8_t *)(mboot->framebuffer_addr);

    drawASCIILogo(0);

    qemu_log("SayoriOS v%d.%d.%d\nBuilt: %s",
             VERSION_MAJOR, VERSION_MINOR, VERSION_PATCH, // Версия ядра
             __TIMESTAMP__                                // Время окончания компиляции ядра
    );

    qemu_log("Bootloader header at: %x", (size_t)mboot);

    qemu_log("SSE: %s", sse_check() ? "Supported" : "Not supported");

    if (sse_check())
    {
        fpu_save();
    }

    qemu_log("Setting `Interrupt Descriptor Table`...");
    init_descriptor_tables();
    qemu_log("Setting `RIH`...");
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

    switch_qemu_logging();

    kHandlerCMD((char *)mboot->cmdline);

    drv_vbe_init(mboot);

    // qemu_log("Audio system init");
    // audio_system_init();

    qemu_log("FSM Init");
    fsm_init();

    qemu_log("Registration of file system drivers...");
    fs_tarfs_register();    
    fs_iso9660_init();
    fsm_reg("FAT32", &fs_fat32_read, &fs_fat32_write, &fs_fat32_info, &fs_fat32_create, &fs_fat32_delete,
        &fs_fat32_dir, &fs_fat32_label, &fs_fat32_detect);
    fs_noctfs_init();
    // fsm_reg("TEMPFS", &fs_tempfs_read, &fs_tempfs_write, &fs_tempfs_info, &fs_tempfs_create, &fs_tempfs_delete,
    //         &fs_tempfs_dir, &fs_tempfs_label, &fs_tempfs_detect);
        
    grub_modules_init(mboot);
        
    fsm_dpm_update(-1);
    kernel_start_time = getTicks();

    mtrr_init();

    qemu_log("Initializing the virtual video memory manager...");
    init_vbe(mboot);

    fonts_init("R:/Sayori/Fonts/UniCyrX-ibm-8x16.psf");

    qemu_log("Initializing Task Manager...");
    // init_task_manager();

    clean_screen();

    qemu_log("Initalizing fonts...");
    tty_fontConfigurate();

    draw_vga_str("Initializing devices...", 23, 0, 0, 0xffffff);

    tty_setcolor(0xffffff);

    // bootScreenInit(15);
    // bootScreenLazy(true);

    keyboard_buffer_init();

    ps2_init();

    ps2_keyboard_init();

    if (ps2_channel2_okay)
    {
        // bootScreenPaint("Настройка PS/2 Мыши...");
        // mouse_install();
    }

    // bootScreenPaint("Пост-настройка PS/2...");
    ps2_keyboard_install_irq();
    // ps2_mouse_install_irq();

    bootScreenPaint("PCI Setup...");
    pci_scan_everything();

    bootScreenPaint("Инициализация ATA...");
    // ata_init();
    // ata_dma_init();

    bootScreenPaint("Калибровка датчика температуры процессора...");
    cputemp_calibrate();

    // bootScreenPaint("Настройка FDT...");
    // file_descriptors_init();

    bootScreenPaint("Настройка системных вызовов...");
    qemu_log("Registering System Calls...");
    init_syscalls();

    bootScreenPaint("Настройка ENV...");
    qemu_log("Registering ENV...");
    // configure_env();

    // bootScreenPaint("Инициализация списка сетевых карт...");
    // netcards_list_init();

    // bootScreenPaint("Инициализация сетевого стека...");
    // netstack_init();
    // bootScreenPaint("Инициализация ARP...");
    // arp_init();
    // bootScreenPaint("Инициализация RTL8139...");
    // rtl8139_init();
    // bootScreenPaint("Инициализация DHCP...");
    // dhcp_init_all_cards();
    bootScreenPaint("Готово...");
    bootScreenClose(0x000000, 0xFFFFFF);
    tty_set_bgcolor(COLOR_BG);

    tty_printf("NocturneOS v%d.%d.%d\nДата компиляции: %s\n",
               VERSION_MAJOR, VERSION_MINOR, VERSION_PATCH, // Версия ядра
               __TIMESTAMP__                                // Время окончания компиляции ядра
    );

    // while(1)
    // ;

    tty_printf("\nВлюбиться можно в красоту, но полюбить - лишь только душу.\n(c) Уильям Шекспир\n");

    sayori_time_t time = get_time();
    tty_printf("\nВремя: %02d:%02d:%02d\n", time.hours, time.minutes, time.seconds);

    // _tty_printf("Listing ATA disks:\n");
    // ata_list();

    tty_taskInit();

    /*
    if (is_rsdp)
    {
        RSDPDescriptor *rsdp = rsdp_find();
        qemu_log("RSDP at: %p", rsdp);

        if (rsdp)
        {
            acpi_scan_all_tables(rsdp->RSDTaddress);

            find_facp(rsdp->RSDTaddress);

            lapic_init(rsdp);
        }
        else
        {
            tty_printf("ACPI not supported! (Are you running in UEFI mode?)\n");
            qemu_err("ACPI not supported! (Are you running in UEFI mode?)");
        }
    }

    tty_printf("Processors: %d\n", system_processors_found);
    */

    // if (test_network)
    // {
    //     _tty_printf("Listing network cards:\n");

    //     uint8_t mac_buffer[6] = {0};

    //     for (int i = 0; i < netcards_get_count(); i++)
    //     {
    //         netcard_entry_t *entry = netcard_get(i);

    //         _tty_printf("\tName: %s\n", entry->name);
    //         entry->get_mac_addr(mac_buffer);

    //         _tty_printf("\tMAC address: %x:%x:%x:%x:%x:%x\n",
    //                     mac_buffer[0],
    //                     mac_buffer[1],
    //                     mac_buffer[2],
    //                     mac_buffer[3],
    //                     mac_buffer[4],
    //                     mac_buffer[5]);
    //     }

    //     _tty_printf("OK!\n");
    // }

    ac97_init();
    ahci_init();

    /// Обновим данные обо всех дисках
    fsm_dpm_update(-1);

    // vio_ntw_init();

    igfx_init();

    //    hda_init();
    // void k();

    //	 create_process(k, "process", false, true);
    //    sleep_ms(500);
    //    create_process(k, "process2", false, true);
    //    sleep_ms(1500);
    //    create_process(k, "process3", false, true);

    rust_main();

    qemu_log("System initialized everything at: %f seconds.", (double)(getTicks() - kernel_start_time) / getFrequency());
    tty_printf("System initialized everything at: %.2f seconds.\n", (double)(getTicks() - kernel_start_time) / getFrequency());

    // char* args[] = {};
    // spawn("R:/hellors", 0, args);

//    void sysidle();
//    thread_create(get_current_proc(), sysidle, 0x100, true, false);

    // net_test();

    new_nsh();

    while (1)
        ;
}

// void net_test() {
//     socket_address_t server_addr = {
//         .address = {192, 168, 1, 128},
//         .port = 9999
//     };

//     char data[32] = {0};

//     socket_t* srv_sock = socket_new(&server_addr, SOCKET_TCP);
//     socket_t* client_sock = socket_listen(srv_sock);

//     if(client_sock) {
//         socket_read_until_newline(client_sock, data);

//         socket_close(client_sock);
//     }

//     int length = strlen(data);

//     qemu_note("Received %d bytes with data: `%s`", length, data);
// }

/*
void sysidle() {
    while(1) {
        __asm__ volatile("hlt");
      yield();
    }
}
*/

// void k() {
//     for(int i = 0; i < 10; i++) {
//         qemu_err("HELLO");
//         sleep_ms(250);
//     }
// }
