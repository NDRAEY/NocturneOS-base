#pragma once

#include <common.h>

#define DPM_ERROR_CANT_MOUNT (-1)  /// Не удалось примонтировать устройство
#define DPM_ERROR_NOT_READY (-2) /// Устройство не готово к работе
#define DPM_ERROR_CANT_READ (-3)   /// Не удалось прочитать файл

// disk, offset_h, offset_l, size, buffer
typedef size_t (*dpm_disk_read_cmd)(size_t, uint64_t, uint64_t, size_t, void *);
typedef size_t (*dpm_disk_write_cmd)(size_t, uint64_t, uint64_t, size_t, const void *);

// disk, data, data_length
typedef size_t (*dpm_disk_command_cmd)(size_t, const void *, size_t);

typedef struct
{
    bool Ready;            /// Устройство подключено?
    char* Name;            /// Имя диск
    char* FileSystem;      /// Файловая система
    int Status;            /// Режим устройства (0 - не обслуживает | 1 - Чтение/Запись | 2 - Только чтение)
    size_t Size;           /// Размер диска (в байтах)
    size_t Sectors;        /// Кол-во секторов
    size_t SectorSize;     /// Размер секторов
    int AddrMode;          /// Метод адрессации (0 - CHS | 1 - LBA | 2 - RAM | 3 - RW for FNC)
    char Serial[16];       /// Серийный номер диска
    void *Point;           /// Точка входа в оперативной памяти
    void *Reserved;        /// Можно в ОЗУ дописать доп.данные если требуется.
    dpm_disk_read_cmd Read;  /// Команда для чтения данных
    dpm_disk_write_cmd Write; /// Команда для записи данных
    dpm_disk_command_cmd Command; /// Команда для командования
} __attribute__((packed)) DPM_Disk;

extern DPM_Disk DPM_Disks[32];

void *dpm_metadata_read(char Letter);
void dpm_metadata_write(char Letter, uint32_t Addr);
size_t dpm_read(char Letter, uint64_t high_offset, uint64_t low_offset, size_t Size, void *Buffer);
size_t dpm_write(char Letter, uint64_t high_offset, uint64_t low_offset, size_t Size, const void *Buffer);
int dpm_reg(char Letter, char *Name, char *FS, int Status, size_t Size, size_t Sectors, size_t SectorSize, int AddrMode, char *Serial, void *Point);
DPM_Disk dpm_info(char Letter);
int dpm_unmount(char Letter, bool FreeReserved);

void dpm_LabelUpdate(char Letter, const char *Label);
void dpm_FileSystemUpdate(char Letter, char *FileSystem);

void dpm_set_read_func(char Letter, dpm_disk_read_cmd Read);
void dpm_set_write_func(char Letter, dpm_disk_write_cmd Write);
void dpm_set_command_func(char Letter, dpm_disk_command_cmd Command);

int dpm_searchFreeIndex(int Index);

void dpm_dump(char Letter);