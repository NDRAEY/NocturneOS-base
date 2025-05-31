#pragma once

#include "common.h"
#include <fs/fsm.h>  
#include <sys/scheduler.h>

#define EOF (-1)
#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2

#define STDIO_ERR_NOT_FOUND		1	// Файл не найден
#define STDIO_ERR_MODE_ERROR	2	// Режим работы не определён
#define STDIO_ERR_SIZE			3	// Размер файла имеет недопустимый размер
#define STDIO_ERR_FILE			4	// Файл не был открыт
#define STDIO_ERR_MEMORY		5	// Memory error
#define STDIO_ERR_NULLPTR		6	// nullptr passed
#define STDIO_ERR_NOTIMPL		7	// Not implemented
#define STDIO_ERR_PERMDENY		8	// Permission denied
#define STDIO_ERR_INVALOP		9	// Invalid operation (e.g. trying to call `eject` on hard drive)

/**
 * @brief Структура файла. Требуется для работы с VFS
 * 
 */
typedef struct FILE {
	char* path;
    uint32_t size;
    uint32_t fmode;
	bool open;
	size_t pos;
	uint32_t err;
} FILE;

// Типы открытого файла, тип флагов rw и т.д.
enum FileOpenMode {
	O_READ = 1,
	O_WRITE = 2,
	O_RDWR = 3,
};

FILE* fopen(const char* filename, size_t mode);
void fclose(FILE *stream);
int32_t fread(FILE* stream, size_t count, size_t size, void* buffer);
int ftell(FILE* stream);
int fsize(FILE *stream);
ssize_t fseek(FILE* stream, ssize_t offset, uint8_t whence);
void rewind(FILE *stream);
uint32_t ferror(FILE* stream);

size_t fwrite(FILE *stream, size_t size, size_t count, const void *ptr);


void getcwd(char* str);