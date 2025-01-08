#pragma once

#define VERSION_MAJOR   0        /// Версия ядра
#define VERSION_MINOR   3        /// Пре-релиз
#define VERSION_PATCH   5        /// Патч
#define ARCH_TYPE       "i386"   /// Архитектура
#define VERNAME         "Soul"   /// Имя версии (изменяется вместе с минорной части версии)
#define SUBVERSIONNAME  "Aura" /// Вторичное имя версии (изменяется вместе с каждым глобальным релизом)


// Макрос для создания версии как строки
#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)
#define VERSION_STRING TOSTRING(VERSION_MAJOR) "." TOSTRING(VERSION_MINOR) "." TOSTRING(VERSION_PATCH)