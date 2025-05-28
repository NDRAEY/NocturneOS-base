/**
 * @file common.h
 * @author Пиминов Никита (nikita.piminoff@yandex.ru)
 * @brief Основные определения ядра
 * @version 0.3.5
 * @date 2023-12-07
 * @copyright Copyright SayoriOS Team (c) 2022-2025
 */
#pragma once

#include <stdbool.h>

#ifndef __cplusplus

#define nullptr ((void*)0)
#define NULL (0)

#endif

#define SAYORI_INLINE static inline __attribute__((always_inline))
#define SAYORI_UNUSED __attribute__((unused))

#define KB (1U << 10)
#define MB (1U << 20)
#define GB (1U << 30)

#define ALIGN(value, align) ((value) + ((-(value)) & ((align) - 1)))
#define IS_ALIGNED(value, align) ((value) % (align) == 0)

/* 64-bit types */
typedef	unsigned long long	uint64_t;
typedef	long long			int64_t;
/* 32-bit types */
typedef	unsigned int	uint32_t;
typedef	int		int32_t;
/* 16-bit types */
typedef	unsigned short	uint16_t;
typedef	short		int16_t;
/* 8-bit types */
typedef	unsigned char	uint8_t;
typedef	signed char		int8_t;

#ifdef SAYORI64
typedef	uint64_t		size_t;
typedef	int64_t			ssize_t;
#else
typedef	uint32_t		size_t;
typedef	int32_t			ssize_t;
#endif

typedef size_t uintptr_t;
typedef size_t ptrdiff_t;

#define allocate_one(T) kcalloc(sizeof(T), 1)

// Use ON_NULLPTR macro to tell a user (developer) that he passed a nullptr
#ifndef RELEASE
#define ON_NULLPTR(ptr, code) \
	do {                         \
		if((ptr) == 0) { \
			qemu_err("You have an illusion that you see an object but you can't touch it because it's not exist..."); \
			code                                               \
		}      \
	} while(0)
#else
#define ON_NULLPTR(ptr, code)
#endif