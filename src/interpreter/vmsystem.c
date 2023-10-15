/*
 * @vmsystem.c
 *
 * @brief
 * @details
 * This is based on other projects:
 *   junkbasic (David Michael Betz): https://github.com/dbetz/junkbasic/
 *   Others (see individual files)
 *
 *   please contact their authors for more information.
 *
 * @author Emiliano Gonzalez (egonzalez . hiperion @ gmail . com))
 * @version 0.1
 * @date 2023
 * @copyright MIT License
 * @see https://github.com/hiperiondev/basic_lang
 */

#include <stdio.h>
#include <stdarg.h>

#include "vmsystem.h"

// allocate low memory from the heap
void* vm_allocate_low_memory(vm_context_t *sys, size_t size) {
    uint8_t *p = sys->nextLow;
    size = (size + ALIGN_MASK) & ~ALIGN_MASK;
    if (p + size > sys->nextHigh)
        vm_system_abort(sys, "insufficient memory");
    sys->nextLow += size;
    if (sys->heapSize - (sys->nextHigh - sys->nextLow) > sys->maxHeapUsed)
        sys->maxHeapUsed = sys->heapSize - (sys->nextHigh - sys->nextLow);
    return p;
}

int vm_getchar(void) {
    int ch = getchar();
    if (ch == '\r')
        ch = '\n';

    return ch;
}

void vm_putchar(int ch) {
    if (ch == '\n')
        putchar('\r');

    putchar(ch);
}

void vm_flush(void) {
    fflush(stdout);
}

void vm_printf(const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    vm_vprintf(fmt, ap);
    va_end(ap);
}

void vm_vprintf(const char *fmt, va_list ap) {
    char buf[80], *p = buf;
    vsprintf(buf, fmt, ap);
    while (*p != '\0')
        vm_putchar(*p++);
}

void vm_system_abort(vm_context_t *sys, const char *fmt, ...) {
    char buf[100], *p = buf;
    va_list ap;
    va_start(ap, fmt);
    vm_printf("error: ");
    vsprintf(buf, fmt, ap);
    while (*p != '\0')
        vm_putchar(*p++);
    vm_putchar('\n');
    va_end(ap);
    longjmp(sys->errorTarget, 1);
}
