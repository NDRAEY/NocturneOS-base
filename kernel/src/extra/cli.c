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
    {"NSH", "nsh", new_nsh, "New NSH"},
    {nullptr, nullptr, nullptr}
};