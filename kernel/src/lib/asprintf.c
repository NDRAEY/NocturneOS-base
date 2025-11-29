#include "lib/asprintf.h"
#include "lib/sprintf.h"
#include "io/logging.h"
#include "mem/vmm.h"

int vasprintf(char **strp, const char *fmt, va_list ap) {
    va_list new_ap;
    
    // Clone `ap`, because first invocation of `vsnprintf` will mutate ap and it
    // will become unusable in second invoation.
#ifdef __GNUC__
    va_copy(new_ap, ap);
#else
    new_ap = ap;
#endif

    const int len = vsnprintf(NULL, 0, fmt, new_ap);
    *strp = kmalloc(len + 1);
    
    vsnprintf(*strp, len + 1, fmt, ap);
    return len;
}

int asprintf(char **strp, const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    const int len = vasprintf(strp, fmt, ap);
    va_end(ap);
    return len;
}