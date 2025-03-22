/**
 * @file extra/cli.c
 * @author Пиминов Никита (nikita.piminoff@yandex.ru), NDRAEY >_ (pikachu_andrey@vk.com)
 * @brief [CLI] Sayori Command Line (SCL -> Shell)
 * @version 0.3.5
 * @date 2022-10-20
 * @copyright Copyright SayoriOS Team (c) 2022-2025
 */

#include <io/ports.h>
#include <sys/variable.h>
#include "elf/elf.h"
#include "io/tty.h"
#include "mem/vmm.h"
#include "io/status_loggers.h"
#include "mem/pmm.h"
#include "lib/split.h"
#include "version.h"
#include "drv/input/keyboard.h"
#include "lib/php/explode.h"
#include "fs/nvfs.h"
#include "lib/time_conversion.h"
#include "lib/list.h"
#include "sys/scheduler.h"
#include "sys/timer.h"
#include "drv/disk/dpm.h"
#include <fmt/tga.h>
#include <sys/cpuinfo.h>
#include "../../include/lib/fileio.h"
#include "sys/system.h"
#include "debug/hexview.h"
#include "lib/command_parser.h"

int G_CLI_CURINXA = 0;
int G_CLI_CURINXB = 0;
int G_CLI_CURINXD = 17; /// Текущий диск
char G_CLI_PATH[1024] = "R:/";

typedef struct
{
    char *name;
    char *alias;
    uint32_t (*funcv)(uint32_t, char **);
    char *helpstring;
} CLI_CMD_ELEM;

CLI_CMD_ELEM G_CLI_CMD[];

uint32_t CLI_CMD_SYSINFO(uint32_t c, char *v[])
{
    size_t cpubrand_length = 0;

    get_cpu_brand(NULL, &cpubrand_length);

    qemu_log("Length is: %d", cpubrand_length);

    char* cpubrand = kcalloc(1, cpubrand_length + 1);

    get_cpu_brand(cpubrand, NULL);

    tty_printf("NocturneOS by SayoriOS & NocturneOS Team (pimnik98 and NDRAEY)\n\n");

    tty_printf("Системная информация:\n");
    tty_printf("\tOS:                      NocturneOS v%d.%d.%d '%s'\n", VERSION_MAJOR, VERSION_MINOR, VERSION_PATCH, VERSION_NAME);
    tty_printf("\tДата сборки:             %s\n", __TIMESTAMP__);
    tty_printf("\tАрхитектура:             %s\n", OS_ARCH);
    tty_printf("\tПроцессор:               %s\n", cpubrand);

    if (is_temperature_module_present())
    {
        tty_printf("\tТемпература:             %d *C\n", get_cpu_temperature());
    }
    else
    {
        tty_printf("\tТемпература:             -- *C\n");
    }

    tty_printf("\tОЗУ:                     %u kb\n", getInstalledRam() / 1024);
    tty_printf("\tВидеоадаптер:            %s\n", "Legacy framebuffer (Unknown)");
    tty_printf("\tДисплей:                 %s (%dx%d)\n", "(\?\?\?)", getScreenWidth(), getScreenHeight());
    tty_printf("\tТики:                    %d\n", getTicks());
    tty_printf("\tЧастота таймера:         %d Гц\n", getFrequency());
    tty_printf("\tВремя с момента запуска: %f секунд\n", getUptime());

    kfree(cpubrand);

    return 1;
}

uint32_t CLI_CMD_DISKPART(uint32_t c, char *v[])
{
    _tty_printf("Список примонтированных дисков:\n");
    for (int i = 0; i < 26; i++)
    {
        DPM_Disk dpm = dpm_info(i + 65);
        if (dpm.Ready != 1)
            continue;
        _tty_printf(" [%c] %s | %s\n", i + 65, dpm.FileSystem, dpm.Name);
    }

    _tty_printf("\n");
    return 1;
}

uint32_t CLI_CMD_RUN(uint32_t c, char *v[])
{
    if (c == 0)
    {
        // tty_setcolor(COLOR_ERROR);
        tty_printf("Файл не указан.\n");
        return 1;
    }

    const char *path = v[0];

    FILE *elf_exec = fopen(path, "r");

    if (!elf_exec)
    {
        tty_error("\"%s\" не является внутренней или внешней\n командой, исполняемой программой или пакетным файлом.\n", path);
        return 2;
    }

    if (!is_elf_file(elf_exec))
    {
        fclose(elf_exec);
        tty_printf("\"%s\" не является программой или данный тип файла не поддерживается.\n", path);
        return 2;
    }

    fclose(elf_exec);

    run_elf_file(path, c, v);

    return 0;
}


uint32_t gfxbench(uint32_t argc, char *args[]);
uint32_t CLI_CMD_NET(uint32_t c, char **v);

uint32_t proc_list(uint32_t argc, char *argv[])
{
    extern list_t process_list;
    extern list_t thread_list;

    tty_printf("%d процессов\n", process_list.count);

    list_item_t *item = process_list.first;
    for (int i = 0; i < process_list.count; i++)
    {

        process_t *proc = (process_t *)item;

        tty_printf("    Процесс: %d [%s]\n", proc->pid, proc->name);

        item = item->next;
    }

    tty_printf("%d потоков\n", thread_list.count);

    list_item_t *item_thread = thread_list.first;
    for (int j = 0; j < thread_list.count; j++)
    {
        thread_t *thread = (thread_t *)item_thread;

        tty_printf("    Поток: #%u процесса #%u; Стек: (%x, %x, %d); Состояние: %s\n",
                   thread->id, thread->process->pid, thread->stack_top, thread->stack, thread->stack_size,
                   thread_state_string(thread->state));

        item_thread = item_thread->next;
    }

    return 0;
}

uint32_t CLI_CMD_REBOOT(uint32_t argc, char *argv[])
{
    reboot();

    return 0;
}

uint32_t CLI_SPAWN(uint32_t argc, char *argv[])
{
    qemu_log("SPAWN! %u", argc);
    if (argc <= 1)
    {
        // tty_setcolor(COLOR_ERROR);
        tty_printf("Файл не указан.\n");
        return 1;
    }

    const char *path = argv[1];

    FILE *elf_exec = fopen(path, "r");

    if (!elf_exec)
    {
        fclose(elf_exec);
        tty_error("\"%s\" не является внутренней или внешней\n командой, исполняемой программой или пакетным файлом.\n", path);
        return 2;
    }

    if (!is_elf_file(elf_exec))
    {
        fclose(elf_exec);
        tty_printf("\"%s\" не является программой или данный тип файла не поддерживается.\n", path);
        return 2;
    }

    fclose(elf_exec);

    spawn(path, argc, argv);

    return 0;
}

uint32_t CLI_SPAWN_TEST(uint32_t argc, char *argv[])
{
    char *cmdline[] = {"hello"};

    spawn("R:\\prog", 0, cmdline);
    sleep_ms(1000);
    spawn("R:\\hellors", 0, cmdline);

    return 0;
}

uint32_t CLI_CMD_MTRR(uint32_t argc, char *argv[])
{
    list_mtrrs();

    return 0;
}

uint32_t CLI_RD(uint32_t argc, char *argv[])
{
    if (argc < 2)
    {
        tty_error("No arguments.\n");
        return 1;
    }

    char *disk = argv[1];
    DPM_Disk data = dpm_info(disk[0]);

    if (!data.Ready)
    {
        tty_error("No disk.\n");
        return 1;
    }

    char *newdata = kcalloc(1024, 1);

    dpm_read(disk[0], 0, 0, 1024, newdata);

    hexview_advanced(newdata, 1024, 26, true, _tty_printf);

    punch();

    kfree(newdata);

    return 0;
}

uint32_t CLI_CMD_HEX(uint32_t argc, char **argv)
{
    if (argc < 2)
    {
        tty_printf("No arguments\n");
        return 1;
    }

    char *file = argv[1];

    FILE *fp = fopen(file, "rb");

    if (!fp)
    {
        tty_error("Failed to open file: %s\n", file);
        return 1;
    }

    size_t sz = fsize(fp);

    char *data = kcalloc(512, 1);

    fread(fp, 512, 1, data);

    tty_printf("Showing first 512 bytes:\n");

    hexview_advanced(data, 512, 26, true, _tty_printf);

    kfree(data);
    fclose(fp);

    return 0;
}

uint32_t CLI_PLAIN(uint32_t argc, char **argv)
{
    if (argc < 3)
    {
        tty_error("plain <address> <file>");
        tty_printf("Note: Address must be in HEX without 0x prefix! Example: CAFEBABE");
        return 1;
    }

    size_t address = htoi(argv[1]);

    qemu_note("Address is: %x", address);

    FILE *file = fopen(argv[2], "rb");

    size_t filesize = fsize(file);

    qemu_note("File size is: %d", filesize);

    void *a = kmalloc_common(ALIGN(filesize, PAGE_SIZE), PAGE_SIZE);
    memset(a, 0, ALIGN(filesize, PAGE_SIZE));

    size_t a_phys = virt2phys(get_kernel_page_directory(), (virtual_addr_t)a);

    map_pages(get_kernel_page_directory(), (physical_addr_t)a_phys, address, ALIGN(filesize, PAGE_SIZE), PAGE_WRITEABLE);

    fread(file, 1, filesize, (void *)a);

    int (*entry)(int, char **) = (int (*)(int, char **))address;

    qemu_log("RESULT IS: %d", entry(0, 0));

    unmap_pages_overlapping(get_kernel_page_directory(), address, filesize);

    kfree(a);
    fclose(file);

    return 0;
}

uint32_t pavi_view(uint32_t, char **);
// uint32_t minesweeper(uint32_t, char**);
uint32_t shell_diskctl(uint32_t, char **);
uint32_t calendar(uint32_t, char **);
uint32_t new_nsh(uint32_t, char**);

CLI_CMD_ELEM G_CLI_CMD[] = {
    // {"CALENDAR", "calendar", calendar, "Календарь"},
    {"DISKCTL", "diskctl", shell_diskctl, "Управление ATA-дисками"},
    {"DISKPART", "diskpart", CLI_CMD_DISKPART, "Список дисков Disk Partition Manager"},
    {"NET", "net", CLI_CMD_NET, "Информация о сетевых устройствах"},
    {"GFXBENCH", "gfxbench", gfxbench, "Тестирование скорости фреймбуфера"},
    // {"MINESWEEPER", "minesweeper", minesweeper, "Сапёр"},
    {"MTRR", "mtrr", CLI_CMD_MTRR, "MTRR"},
    {"PROC", "proc", proc_list, "Список процессов"},
    {"SYSINFO", "sysinfo", CLI_CMD_SYSINFO, "Информация о системе"},
    {"REBOOT", "reboot", CLI_CMD_REBOOT, "Перезагрузка"},
    {"RD", "rd", CLI_RD, "Чтение данных с диска"},
    {"SPAWN", "spawn", CLI_SPAWN, "spawn a new process"},
    {"HEX", "hex", CLI_CMD_HEX, "Show hex data"},
    {"PLAIN", "plain", CLI_PLAIN, "Run plain program"},
    {"NSH", "nsh", new_nsh, "New NSH"},
    {nullptr, nullptr, nullptr}
};

void cli_handler(const char *ncmd)
{
    set_cursor_enabled(0);

    command_parser_t parser = {};

    command_parser_new(&parser, ncmd);

    for (size_t i = 0; i < parser.argc; i++)
    {
        qemu_log("[CLI] '%s' => argc: %d => argv: %s", ncmd, i, parser.argv[i]);
    }

    bool found = false;

    for (size_t i = 0; G_CLI_CMD[i].name != nullptr; i++)
    {
        if (strcmpn(G_CLI_CMD[i].name, parser.argv[0]) || strcmpn(G_CLI_CMD[i].alias, parser.argv[0]))
        {
            G_CLI_CMD[i].funcv(parser.argc, parser.argv);
            found = true;
            break;
        }
    }

    if (!found)
    {
        CLI_CMD_RUN(parser.argc, parser.argv);
    }

    command_parser_destroy(&parser);

    set_cursor_enabled(1);
}

void cli()
{
    qemu_log("[CLI] Started...");
    tty_set_bgcolor(0xFF000000);
    tty_setcolor(0xFFFFFF);

    _tty_printf("NocturneOS [Версия: v%d.%d.%d]\n", VERSION_MAJOR, VERSION_MINOR, VERSION_PATCH);
    _tty_printf("(c) SayoriOS & NocturneOS Team, 2025.\nДля дополнительной информации наберите \"help\".\n\n");

    punch();

    char *input_buffer = kcalloc(1, 1024);
    while (1)
    {
        size_t memory_cur = system_heap.used_memory;
        size_t memory_cnt_cur = system_heap.allocated_count;

        tty_setcolor(0xFFFFFF);
        tty_printf("%s>", G_CLI_PATH);
        memset(input_buffer, 0, 512);

        gets(input_buffer);
        tty_printf("\n");

        size_t len_cmd = strlen(input_buffer);
        if (len_cmd == 0)
        {
            continue;
        }

        size_t current_time = timestamp();
        qemu_log("cmd: %s", input_buffer);

        cli_handler(input_buffer);

        ssize_t delta = (int)system_heap.used_memory - (int)memory_cur;
        ssize_t delta_blocks = (int)system_heap.allocated_count - (int)memory_cnt_cur;
        qemu_warn("Used memory before: %d (%d blocks)", memory_cur, memory_cnt_cur);
        qemu_warn("Used memory now: %d (%d blocks)", system_heap.used_memory, system_heap.allocated_count);
        qemu_warn("Memory used: %d (%d blocks)", delta, delta_blocks);

        if (delta > 0)
        {
            qemu_err("Memory leak!");
        }
        else if (delta == 0)
        {
            qemu_ok("All right! No memory leaks! Keep it up, buddy!");
        }

        qemu_note("Time elapsed: %d milliseconds", timestamp() - current_time);
    }

    kfree(input_buffer);
}
