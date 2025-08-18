#pragma once

#include <common.h>

#define FSM_MOD_READ 0x01u  /// Права чтения
#define FSM_MOD_WRITE 0x02u /// Права записи
#define FSM_MOD_EXEC 0x04u  /// Права выполнения

typedef enum {
	TYPE_FILE = 0,
	TYPE_DIR = 5
} FSM_ENTITY_TYPE;

typedef struct
{
	uint16_t year;
	uint8_t month;
	uint8_t day;
	uint8_t hour;
	uint8_t minute;
	uint8_t second;
} __attribute__((packed)) FSM_TIME;

typedef struct {
	bool Ready;		   /// Существует ли файл?
	char Name[1024];   /// Имя файла
	char Path[1024];   /// Путь файла
	int Mode;		   /// Режим файла
	size_t Size;	   /// Размер файла в байтах (oсt2bin)
	FSM_TIME LastTime; /// Время последнего изменения файла
	FSM_ENTITY_TYPE Type;		   /// Тип элемента
	uint32_t CHMOD;	   /// CHMOD файла
} __attribute__((packed)) FSM_FILE;

typedef struct
{
	bool Ready;		   /// Существует ли файл?
	size_t CountFiles; /// Количество файлов
	size_t CountDir;   /// Количество папок
	size_t CountOther; /// Количество неизвестного типа файлов
	FSM_FILE *Files;   /// Файлы и папки
} __attribute__((packed)) FSM_DIR;

/// Буква, Название, откуда, сколько, буфер
typedef size_t (*fsm_cmd_read_t)(const char* disk_name, const char *name, size_t offset, size_t count, void *buffer);

/// Буква, Название, куда, сколько, буфер
typedef size_t (*fsm_cmd_write_t)(const char* disk_name, const char *path, size_t offset, size_t count, const void *buffer);

/// Буква, Название
typedef FSM_FILE (*fsm_cmd_info_t)(const char* disk_name, const char *path);

/// Буква, Название
typedef void (*fsm_cmd_dir_t)(const char* disk_name, const char* path, FSM_DIR *out);

/// Буква, Название, Тип (0 - файл | 1 - папка)
typedef int (*fsm_cmd_create_t)(const char* disk_name, const char *path, FSM_ENTITY_TYPE type);

/// Буква, Название, Тип (0 - файл | 1 - папка)
typedef int (*fsm_cmd_delete_t)(const char* disk_name, const char *path, FSM_ENTITY_TYPE type);

/// Буква, Буфер
typedef void (*fsm_cmd_label_t)(const char* disk_name, char *buffer);

/// Буква, Буфер
typedef int (*fsm_cmd_detect_t)(const char* disk_name);

typedef struct
{
	bool Ready;				 /// Загружена ли фс?
	char* Name;			 /// Наименование драйвера
	fsm_cmd_read_t Read;	 /// Команда для чтения
	fsm_cmd_write_t Write;	 /// Команда для записи
	fsm_cmd_info_t Info;	 /// Команда для получения информации
	fsm_cmd_dir_t Dir;		 /// Команда для получения информации о папке
	fsm_cmd_create_t Create; /// Команда для создания файла или папка
	fsm_cmd_delete_t Delete; /// Команда для удаления файла или папка

	fsm_cmd_label_t Label;	 /// Команда для получения имени диска
	fsm_cmd_detect_t Detect; /// Команда для определения, предналежит ли диск к фс
	void *Reserved;			 /// Можно в ОЗУ дописать доп.данные если требуется.
} FilesystemHandler;

typedef struct {
	char* diskman_disk_id;
	char* filesystem_name;
} FSM_Mount;

void fsm_init();
int fsm_getIDbyName(const char *Name);
size_t fsm_read(int FIndex, const char* disk_name, const char *Name, size_t Offset, size_t Count, void *Buffer);
size_t fsm_write(int FIndex, const char* disk_name, const char *Name, size_t Offset, size_t Count, const void *Buffer);
FSM_FILE fsm_info(int FIndex, const char* disk_name, const char *Name);
void fsm_reg(const char *Name, fsm_cmd_read_t Read, fsm_cmd_write_t Write, fsm_cmd_info_t Info, fsm_cmd_create_t Create, fsm_cmd_delete_t Delete, fsm_cmd_dir_t Dir, fsm_cmd_label_t Label, fsm_cmd_detect_t Detect);
int fsm_delete(int FIndex, const char* disk_name, const char *Name, int Mode);
int fsm_create(int FIndex, const char* disk_name, const char *Name, int Mode);

#ifndef RELEASE
void fsm_dump(FSM_FILE file);
#endif

void fsm_dir(int FIndex, const char* disk_name, const char *Name, FSM_DIR *out);
void fsm_dpm_update(const char* disk_id);

const char* fsm_get_disk_filesystem(const char* disk_id);
void fsm_detach_fs(const char* disk_id);