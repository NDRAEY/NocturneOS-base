/**
 * @file lib/stdio.c
 * @authors Пиминов Никита (nikita.piminoff@yandex.ru), NDRAEY >_ (pikachu_andrey@vk.com)
 * @brief Функции для работы с файлами
 * @version 0.4.2
 * @date 2022-11-01
 * @copyright Copyright SayoriOS Team (c) 2022-2025
 */

#include "io/logging.h"
#include "mem/vmm.h"
#include <lib/stdio.h>
#include <fs/fsm.h>
#include <fs/nvfs.h>
#include <io/tty.h>
#include <sys/scheduler.h>

/**
 * @brief Проверка файла на наличие ошибок при работе
 *
 * @param stream Поток (файл)
 */
void fcheckerror(FILE* stream){
	ON_NULLPTR(stream, {
		qemu_log("stream is nullptr!");
		return;
	});
	
	FSM_FILE finfo = nvfs_info(stream->path);
    fsm_file_close(&finfo);

	if (finfo.Ready == 0){
		stream->err = STDIO_ERR_NOT_FOUND;
	} else if (stream->fmode == 0){
		stream->err = STDIO_ERR_MODE_ERROR;
	} else if (stream->size <= 0){
		stream->err = STDIO_ERR_SIZE;
	} else if (stream->open == 0){
		stream->err = STDIO_ERR_FILE;
	}
}

/**
 * @brief Получение кода ошибки
 *
 * @param stream Поток (файл)
 *
 * @return Если возращает 0, значит все в порядке
 */
uint32_t ferror(FILE* stream){
	return stream->err;
}

/**
 * @brief Открывает файл
 *
 * @param filename Путь к файлу
 * @param mode Режим работы
 *
 * @return Структура FILE*
 */

FILE* fopen(const char* filename, size_t mode) {
	ON_NULLPTR(filename, {
               qemu_log("Filename is nullptr!");
               return NULL;
       });

	qemu_log("Open file");
	qemu_log("|- Name: '%s'", filename);
	qemu_log("|- Mode: '%x'", mode);

	FILE* file = kcalloc(sizeof(FILE), 1);
	// Получаем тип открытого файла
	FSM_FILE finfo = nvfs_info(filename);

	if(finfo.Ready == 0 && (mode & O_CREATE)) {
		nvfs_create(filename, TYPE_FILE);

		finfo = nvfs_info(filename);
	}

	if (finfo.Ready == 0 || mode == 0) {
		kfree(file);

		qemu_err("Failed to open file: %s (Exists: %d; FMODE: %d)",
				filename,
				finfo.Ready,
				mode);
		return 0;
	}

	file->open = 1;         // Файл успешно открыт
	file->fmode = mode;     // Режим работы с файлом
	file->size = finfo.Size;                // Размер файла
	file->path = kcalloc(strlen(filename) + 1, 1); // Полный путь к файлу
	file->pos = 0; // Установка указателя в самое начало
	file->err = 0; // Ошибок в работе нет

	
	memcpy(file->path, filename, strlen(filename));
	
    fsm_file_close(&finfo);

	qemu_ok("File opened!");

	return file;
}

/**
 * @brief Закончить работу с файлом
 *
 * @param stream Поток (файл)
 */
void fclose(FILE* stream){
	if(stream) {
		kfree(stream->path);
		kfree(stream);
	}
}

/**
 * @brief Получение размера файла в байтах
 *
 * @param stream Поток (файл)
 *
 * @return Размер файла, в противном случае -1
 */
int fsize(FILE* stream){
	ON_NULLPTR(stream, {
		return -1;
	});

	if (!stream->open || stream->size <= 0 || stream->fmode == 0){
		fcheckerror(stream);
		return -1;
	} else {
		return stream->size;
	}
}

/**
 * @brief Чтение файла
 *
 * @param stream - Поток (файл)
 * @param count - Количество элементов размера size
 * @param size - Сколько читаем таких элементов?
 * @param buffer - Буфер
 *
 * @return Размер прочитаных байтов или -1 при ошибке
 */
int fread(FILE* stream, size_t count, size_t size, void* buffer){
	ON_NULLPTR(stream, {
		return -1;
	});

	ON_NULLPTR(buffer, {
		return -1;
	});
	
	FSM_FILE finfo = nvfs_info(stream->path);
	
	if (!stream->open || finfo.Ready == 0 || stream->size <= 0 || stream->fmode == 0){
		// Удалось ли открыть файл, существует ли файл, размер файла больше нуля и указан правильный режим для работы с файлом
		fcheckerror(stream);
		return -1;
	}

	fsm_file_close(&finfo);

	size_t res = nvfs_read(stream->path, stream->pos, size*count, buffer);

	if(res > 0) {
		stream->pos += size*count;
	}

	return res;
}

/**
 * @brief Текущая позиция считывания в файле
 *
 * @param stream - Поток (файл)
 *
 * @return Возращает позицию или отрицательное значение при ошибке
 */
int ftell(FILE* stream) {
	ON_NULLPTR(stream, {
		return -1;
	});

	if (!stream->open || stream->fmode == 0) {
		qemu_err("ftell(): invalid stream (open: %d; size: %d; mode: %x)", stream->open, stream->size, stream->fmode);
		fcheckerror(stream);
		return -1;
	}

	qemu_warn("Position is: %d", stream->pos);

	return (int)stream->pos;
}

/**
 * @brief Установка позиции в потоке данных относительно текущей позиции
 *
 * @param stream - Поток (файл)
 * @param offset - Смещение позиции
 * @param whence - Точка отсчета смещения
 *
 * @return Если возращает 0, значит все в порядке
 */
ssize_t fseek(FILE* stream, ssize_t offset, uint8_t whence){
	ON_NULLPTR(stream, {
		return -1;
	});

	if (!stream->open || stream->size == 0 || stream->fmode == 0){
		fcheckerror(stream);
		qemu_err("Seek error: Open: %d; Size: %d; Mode: %x", stream->open, stream->size, stream->fmode);
		return -1;
	}

	size_t lsk = 0;

	if (whence == SEEK_CUR) {
		lsk = stream->pos;
	} else if (whence == SEEK_END) {
		lsk = stream->size;
	} else if (whence == SEEK_SET) {
		if(offset >= 0 && (size_t)offset <= stream->size) {
			stream->pos = offset;
			return 0;
		} else {
			qemu_err("Invalid offset (whence = 0x0): %x", offset);
		}
	} else {
		qemu_err("Invalid whence: %d", whence);
		return -1;
	}

	if (lsk + offset > 0 && stream->size >= lsk+offset){
		stream->pos = lsk + offset;
	}
	
	return 0;
}

/**
 * @brief Установка позиции потока в самое начало
 *
 * @param stream - Поток (файл)
 */
void rewind(FILE* stream){
	ON_NULLPTR(stream, {
		return;
	});

	if (!stream->open || stream->size <= 0 || stream->fmode == 0){
		fcheckerror(stream);
	}
	stream->pos = 0;
}

/**
 * @brief Запись файла
 * @param stream Объект файла
 * @param size Размер в байтах
 * @param count Количество объектов размера 'size'
 * @param ptr Буфер
 * @return Количество записаных байт
 */
size_t fwrite(FILE *stream, size_t size, size_t count, const void *ptr) {
	ON_NULLPTR(stream, {
		qemu_log("stream is nullptr!");
		return 0;
	});
	
	if(stream->pos + (size * count) > stream->size) {
		qemu_warn("Out of bounds write!");
	
		// TODO: Filesystem should handle this situation and allocate needed space.
		
		// WARNING: Workaround
		stream->size = stream->pos + (size * count);
	}

	size_t res = nvfs_write(stream->path, stream->pos, size*count, ptr);

	if(res > 0)
		stream->pos += size*count;

	return -1;
}


void getcwd(char* str) {
	char* cwd = get_current_proc()->cwd;

	size_t len = strlen(cwd);
	
	memcpy(str, cwd, len);
}

// TODO: Relative paths
void chdir(const char* str) {
	process_t* proc = get_current_proc();
	
	char* cwd = proc->cwd;

	kfree(cwd);

	proc->cwd = strdynamize(str);

	qemu_printf("Now path is `%s`\n", proc->cwd);
}
