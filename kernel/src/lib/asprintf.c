#include "lib/asprintf.h"
#include "lib/sprintf.h"
#include "mem/vmm.h"

int vasprintf(char **strp, const char *fmt, va_list ap) {
    int len = vsnprintf(NULL, 0, fmt, ap);
    *strp = kmalloc(len + 1);
    vsnprintf(*strp, len + 1, fmt, ap);
    return len;
}

int asprintf(char **strp, const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    int len = vasprintf(strp, fmt, ap);
    va_end(ap);
    return len;
}