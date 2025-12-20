/**
 * @file lib/string.c
 * @author Пиминов Никита (nikita.piminoff@yandex.ru), NDRAEY >_ (pikachu_andrey@vk.com)
 * @brief Функции для работы со строками
 * @version 0.4.2
 * @date 2022-10-01
 * @copyright Copyright SayoriOS Team (c) 2022-2025
 */
#include "common.h"
#include "lib/string.h"
#include "lib/math.h"
#include "mem/vmm.h"
//#include <emmintrin.h>  // SSE functions and types

size_t strlen(const char* input) {
    register size_t len = 0;

    while(*(input + len)) {
        len += 1;
    }

    return len;
}

int strcmp(const char* stra, const char* strb) {
    while(*stra && *stra == *strb) {
        stra++;
        strb++;
    }

    return *stra - *strb;
}

char* strcpy(char* dest, const char* src) {
    size_t i = 0;

    while(src[i]) {
        dest[i] = src[i];
        i++;
    }

    dest[i] = 0;

    return dest;
}

char* strncpy(char* dest, const char* src, size_t n) {
    for(size_t i = 0; i < n; i++) {
        if (src[i] == 0) {
            break;
        }

        dest[i] = src[i];
	}

	return dest;
}

/**
 * @brief Проверяет, является ли символ формата UTF-8
 *
 * @param с - Символ
 *
 * @return bool - true если да
 */
bool isUTF(char c) {
    if (c == -47 || c == -48){
        return true;
    }
    return false;
}

/**
 * @brief Возращает длину строки с учетом UTF-8
 *
 * @param str - Строка
 *
 * @return size_t - Длину символов
 */
size_t mb_strlen(const char *str){
    size_t len = 0;
    const size_t def = strlen(str);

    for(size_t i = 0; i < def;i++){
        if (isUTF(str[i])) continue;
        len++;
    }
    return len;
}


/**
 * @brief Копирование непересекающихся массивов используя SSE
 *
 * @param dest - Указатель на массив в который будут скопированы данные.
 * @param src - Указатель на массив источник копируемых данных.
 * @param size - Количество байт для копирования
 */
/*void sse_memcpy(void* restrict dest, const void* restrict src, size_t size) {
    __m128i* d = (__m128i*)dest;
    const __m128i* s = (const __m128i*)src;

    size_t num_elems = size / sizeof(__m128i);
    const size_t remaining_bytes = size % sizeof(__m128i);

	_mm_prefetch(s, _MM_HINT_NTA);

    while(num_elems--) {
		_mm_storeu_si128(d++, _mm_loadu_si128(s++));
    }

	// Copy remaining bytes
    if (remaining_bytes > 0) {
        char* d_char = (char*)d;
        const char* s_char = (const char*)s;

        for (size_t i = 0; i < remaining_bytes; i++) {
            *d_char++ = *s_char++;
        }
    }

	_mm_sfence();
}*/

/**
 * @brief Копирование непересекающихся массивов
 *
 * @param destination - Указатель на массив в который будут скопированы данные.
 * @param source - Указатель на массив источник копируемых данных.
 * @param n - Количество байт для копирования
 */
void* memcpy(void *restrict destination, const void *restrict source, size_t n){
    size_t *tmp_dest = (size_t *)destination;
    size_t *tmp_src = (size_t *)source;
    size_t len = n / sizeof(size_t);
    size_t i = 0;
    size_t tail = n & (sizeof(size_t) - 1);

    for (; i < len; i++) {
        *tmp_dest++ = *tmp_src++;
    }

    if(tail) {
        char *dest = (char *)destination;
        const char *src = (const char *)source;

        for(i = n - tail; i < n; i++) {
            dest[i] = src[i];
        }
    }

	return destination;
}

#include <io/logging.h>

/**
 * @brief Заполнение массива указанными символами
 *
 * @param ptr - Указатель на заполняемый массив
 * @param value - Код символа для заполнения
 * @param size_t num - Размер заполняемой части массива в байтах
 */
void* memset(void* ptr, int value, size_t num) {
	uint8_t* mem = (uint8_t*)ptr;
    uint8_t byte = (uint8_t)value;

    // Alignment.
    // Example:
    //     ptr = 0x2001
    //     num = whatever
    //     value = whatever
    // Let's assume we have 32-bit target.
    // So the aligned address would be ALIGN(ptr, sizeof(size_t)) = 0x2004.
    // The diff is 0x2004 - 0x2001 = 3. We fill those 3 bytes with our `value`.
    // After this operation: ptr = 0x2004. `ptr` is aligned and we can do aligned writes on that area.
    // By the condition the loop will also break if num == 0, so preventing code do out-of-bounds writes.
    while (((size_t)mem & (sizeof(size_t) - 1)) != 0 && num > 0) {
        *mem++ = byte;
        num--;
    }
    
    // Determine if can we fill memory region with large machine words first.
    if(num >= sizeof(size_t)) {
        // In x86 traditions "word" is 16-bit value,
        // but for this case let's call it a max num of bits that one register of CPU can handle.
        size_t word = 0;
        
        // Fill all the bits of word with our 8-bit value
        // Example for 32-bit:
        // 0x67 -> 0x67676767
        // Example for 64-bit:
        // 0x67 -> 0x6767676767676767

        // We don't have initialize `word`, if `byte` is zero, because `word` is already initialized to zero.
        if(byte != 0) {
            // 8 is not a magic number: 8 bits in bytes.
            for (size_t i = 0; i < sizeof(size_t); i++) {
                word |= ((size_t)byte << (i * 8));
            }
        }

        // Write words.
        volatile size_t* word_ptr = (volatile size_t*)mem;
        while (num >= sizeof(size_t)) {
            *word_ptr++ = word;
            num -= sizeof(size_t);
        }
        
        mem = (uint8_t*)word_ptr;
    }

	while(num--) {
		*mem++ = byte;
	}

	return ptr;
}

/**
 * @brief Копирование массивов (в том числе пересекающихся)
 *
 * @param dest - Указатель на массив в который будут скопированы данные.
 * @param src - Указатель на массив источник копируемых данных
 * @param count - Количество байт для копирования
 */
void* memmove(void *dest, void *src, size_t count) {
	void * ret = dest;
	if (dest <= src || (char*)dest >= ((char*)src + count))
	{
		while (count--)
		{
			*(char*)dest = *(char*)src;
			dest = (char*)dest + 1;
			src = (char*)src + 1;
		}
	}
	else
	{
		dest = (char*)dest + count - 1;
		src = (char*)src + count - 1;
		while (count--)
		{
			*(char*)dest = *(char*)src;
			dest = (char*)dest - 1;
			src = (char*)src - 1;
		}
	}
	return ret;
}

/**
 * @brief Сравнение массивов
 *
 * @param s1 - Указатель на строку
 * @param s2 - Указатель на строку
 * @param n - Размер сравниваемой части массива в байтах.
 *
 * @return int - Возращает 0 если строки идентичны или разницу между ними
 */
int32_t memcmp(const char *s1, const char *s2, size_t n){
    unsigned char u1, u2;

    for (; n--; s1++, s2++){
        u1 = *(unsigned char *)s1;
        u2 = *(unsigned char *)s2;
        if (u1 != u2){
            return (u1 - u2);
        }
    }
    return 0;
}

/**
 * @brief Определение максимальной длины участка строки, содержащего только указанные символы
 *
 * @param s - Указатель на строку, в которой ведется поиск
 * @param accept - Указатель на строку с набором символов, которые должны входить в участок строки str
 *
 * @return Длина начального участка строки, содержащая только символы, указанные в аргументе sym
 */
size_t strspn(const char *s, const char *accept){
    const char *p;
    const char *a;
    size_t count = 0;

    for (p = s; *p != '\0'; ++p){
        for (a = accept; *a != '\0'; ++a){
            if (*p == *a){
                break;
            }
        }
        if (*a == '\0') {
            return count;
        }

        ++count;
    }
    return count;
}

/**
 * @brief Сравнение строк с ограничением количества сравниваемых символов
 *
 * @param s1 - Строка 1
 * @param s2 - Строка 2
 *
 * @return int - Возращает 0 если строки идентичны или разницу между ними
 */
int32_t strncmp(const char *s1, const char *s2, size_t num){
    for (size_t i = 0; i < num; i++){
        if (s1[i] != s2[i]){
            return 1;
        }
    }
    return 0;
}

/**
 * @brief Объединение строк
 *
 * @param s - Указатель на массив в который будет добавлена строка
 * @param t - Указатель на массив из которого будет скопирована строка
 *
 * @return char* - Функция возвращает указатель на массив, в который добавлена строка
 */
char* strcat(char* destination, const char* source) {
    char* ptr = destination;

    // Находим конец строки в destination
    while (*ptr != '\0') {
        ptr++;
    }

    // Копируем символы из source в конец destination
    while (*source != '\0') {
        *ptr = *source;
        ptr++;
        source++;
    }

    // Добавляем завершающий нулевой символ в destination
    *ptr = '\0';

    return destination;
}

/**
 * @brief Вырезает и возвращает подстроку из строки
 *
 * @param dest - Указатель куда будет записана строка
 * @param source - Указатель на исходную строку
 * @param from - Откуда копируем
 * @param length - Количество копируемых байт
 */
void substr(char* restrict dest, const char* restrict source, int from, int length){
    strncpy(dest, source + from, length);
    dest[length] = 0;
}

/**
 * @brief Поиск первого вхождения символа в строку
 *
 * @param _s - Указатель на строку, в которой будет осуществляться поиск.
 * @param _c - Код искомого символа
 *
 * @return char* - Указатель на искомый символ, если он найден в строке str, иначе nullptr.
 */
char *strchr(const char *_s, char _c){
    while (*_s != (char)_c){
        if (!*_s++){
            return 0;
        }
    }
    return (char *)_s;
}

/**
 * @brief Проверяет, является ли строка числом
 *
 * @param c - Указатель на строку.
 *
 * @return bool - если строка является числом
 */
bool isNumber(const char* c) {
    for(uint32_t i = 0, len = strlen(c); i < len; i++){
        if (!(c[i] >= '0' && c[i] <= '9')) {
            return false;
        }
    }
    return true;
}

/**
 * @brief Превращает строку в число
 *
 * @param s - Указатель на строку.
 *
 * @return int - Число
 */
int atoi(const char s[]){
    int i = 0, n = 0;
    bool minus = *s == '-';

    if(minus)
        i++;

    for (; s[i] >= '0' && s[i] <= '9'; ++i)
        n = (n * 10) + (s[i] - '0');

    n *= minus ? -1 : 1;

    return n;
}

size_t htoi(const char* hex) {
    const char* p = hex;
    size_t result = 0;

    while (*p != '\0') {
        if (*p >= '0' && *p <= '9') {
            result = (result << 4) + (*p - '0');
        } else if (*p >= 'A' && *p <= 'F') {
            result = (result << 4) + (*p - 'A' + 10);
        } else if (*p >= 'a' && *p <= 'f') {
            result = (result << 4) + (*p - 'a' + 10);
        } else {
            break;
        }
        p++;
    }

    return result;
}

#ifdef NOCTURNE_SUPPORT_TIER1
char* strdynamize(const char* str) {
    size_t len = strlen(str);

    char* mem = kmalloc(len + 1);
    strcpy(mem, str);

    return mem;
}
#endif