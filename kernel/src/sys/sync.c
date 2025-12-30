/**
 * @file sys/sync.c
 * @author Пиминов Никита (nikita.piminoff@yandex.ru)
 * @brief Примитивы синхронизации
 * @version 0.4.3
 * @date 2022-10-01
 * @copyright Copyright SayoriOS Team (c) 2022-2026
 */
#include	"sys/sync.h"
#include "sys/scheduler.h"

// https://github.com/dreamportdev/Osdev-Notes/blob/master/05_Scheduling/04_Locks.md

/**
 * @brief Получить мьютекс
 * 
 * @param mutex - Мьютекс
 */
void mutex_get(mutex_t* mutex) {
    while (__atomic_test_and_set(&mutex->lock, __ATOMIC_ACQUIRE))
        ;
        // yield();
}

/**
 * @brief Получить ближайщий свободный блок
 * 
 * @param mutex - Мьютекс
 */
void mutex_release(mutex_t* mutex) {
    __atomic_clear(&mutex->lock, __ATOMIC_RELEASE);
}