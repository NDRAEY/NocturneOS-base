/**
 * @file lib/fileio.c
 * @authors Пиминов Никита (nikita.piminoff@yandex.ru), NDRAEY >_ (pikachu_andrey@vk.com)
 * @brief Функции для работы с файлами и папками
 * @version 0.4.2
 * @date 2022-11-01
 * @copyright Copyright SayoriOS Team (c) 2022-2025
 */

#include "io/ports.h"
#include "mem/vmm.h"
#include "../../include/lib/fileio.h"
#include "../../include/fs/fsm.h"
#include "../../include/fs/nvfs.h"


/**
 * @brief [FileIO] Проверяет существует ли сущность и является ли она файлом
 *
 * @param Path - Путь
 *
 * @return bool - true - если успешно, в противном случае false
 */
bool is_file(const char* Path){
    FSM_FILE file = nvfs_info(Path);
    
    if (file.Ready != 1) return false;

    // Free resources immediately - we don't need name and path pointers.
    fsm_file_close(&file);

    if (file.Type != 0) return false;
    return true;
}


/**
 * @brief [FileIO] Проверяет существует ли сущность и является ли она папкой
 *
 * @param Path - Путь
 *
 * @return bool - true - если успешно, в противном случае false
 */
bool is_dir(const char* Path){
    FSM_FILE file = nvfs_info(Path);
    
    if (file.Ready != 1) return false;
    
    // Free resources immediately - we don't need name and path pointers.
    fsm_file_close(&file);
    
    if (file.Type != 5) return false;
    return true;
}


/**
 * @brief [FileIO] Проверяет существует ли сущность
 *
 * @param Path - Путь
 *
 * @return bool - true - если успешно, в противном случае false
 */
bool file_exists(const char* Path){
    FSM_FILE file = nvfs_info(Path);
    
    if (file.Ready != 1) return false;
    
    fsm_file_close(&file);
    
    return true;
}

/**
 * @brief [FileIO] Возвращает размер указанного файла
 *
 * @param Path - Путь
 *
 * @return size_t - Размер в байтах
 */
size_t filesize(const char* Path){
    FSM_FILE file = nvfs_info(Path);
    if (file.Ready != 1) return 0;
    
    // Free resources immediately - we don't need name and path pointers.
    fsm_file_close(&file);
    
    if (file.Type != 0) return 0;
    
    return file.Size;
}

/**
 * @brief [FileIO] Проверяет права чтения у сущности
 *
 * @param Path - Путь
 *
 * @return bool - true - если успешно, в противном случае false
 */
bool is_readable(const char* Path){
    FSM_FILE file = nvfs_info(Path);
    if (file.Ready != 1) return false;
    
    // Free resources immediately - we don't need name and path pointers.
    fsm_file_close(&file);
    
    if (file.CHMOD & FSM_MOD_READ) {
        return true;
    }
    
    return false;
}


/**
 * @brief [FileIO] Проверяет права записи у сущности
 *
 * @param Path - Путь
 *
 * @return bool - true - если успешно, в противном случае false
 */
bool is_writable(const char* Path){
    FSM_FILE file = nvfs_info(Path);
    if (file.Ready != 1) return false;
    
    // Free resources immediately - we don't need name and path pointers.
    fsm_file_close(&file);
    
    if (file.CHMOD & FSM_MOD_WRITE) {
        return true;
    }
    return false;
}


/**
 * @brief [FileIO] Проверяет права выполнения у сущности
 *
 * @param Path - Путь
 *
 * @return bool - true - если успешно, в противном случае false
 */
bool is_executable(const char* Path){
    FSM_FILE file = nvfs_info(Path);
    if (file.Ready != 1) return false;
    
    // Free resources immediately - we don't need name and path pointers.
    fsm_file_close(&file);
    
    if (file.CHMOD & FSM_MOD_EXEC) {
        return true;
    }
    return false;
}


/**
 * @brief [FileIO] Возвращает информацию о правах доступа на сущность
 *
 * @param Path - Путь
 *
 * @return uint32_t Возвращает права доступа
 */
uint32_t fileperms(const char* Path){
    FSM_FILE file = nvfs_info(Path);
    if (file.Ready != 1) return false;
    
    // Free resources immediately - we don't need name and path pointers.
    fsm_file_close(&file);

    uint32_t ret = 0;
    if (file.CHMOD & FSM_MOD_READ) {
        ret |= FSM_MOD_READ;
    }
    if (file.CHMOD & FSM_MOD_EXEC) {
        ret |= FSM_MOD_EXEC;
    }
    if (file.CHMOD & FSM_MOD_WRITE) {
        ret |= FSM_MOD_WRITE;
    }
    return (ret * 100) + (ret * 10) + ret;
}


/**
 * @brief [FileIO] Создает файл
 *
 * @param Path - Путь
 *
 * @return bool - true - если успешно, в противном случае false
 */
bool touch(const char* Path){
    FSM_FILE file = nvfs_info(Path);
    
    if (file.Ready == 1) {
        // Free resources immediately - we don't need name and path pointers.
        fsm_file_close(&file);
        return false;
    }

    return nvfs_create(Path, TYPE_FILE);
}


/**
 * @brief [FileIO] Создает папку
 *
 * @param Path - Путь
 *
 * @return bool - true - если успешно, в противном случае false
 */
bool mkdir(const char* Path){
    return nvfs_create(Path, TYPE_DIR);
}

/**
 * @brief [FileIO] Удаляет файл
 *
 * @param Path - Путь
 *
 * @return bool - true - если успешно, в противном случае false
 */
bool unlink(const char* Path){
    return nvfs_delete(Path, 0);
}

/**
 * @brief [FileIO] Удаляет папку
 *
 * @param Path - Путь
 *
 * @return bool - true - если успешно, в противном случае false
 */
bool rmdir(const char* Path){
    return nvfs_delete(Path, 1);
}