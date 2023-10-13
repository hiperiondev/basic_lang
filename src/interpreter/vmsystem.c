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

// AllocateLowMemory - allocate low memory from the heap
void* AllocateLowMemory(System_t *sys, size_t size) {
    uint8_t *p = sys->nextLow;
    size = (size + ALIGN_MASK) & ~ALIGN_MASK;
    if (p + size > sys->nextHigh)
        Abort(sys, "insufficient memory");
    sys->nextLow += size;
    if (sys->heapSize - (sys->nextHigh - sys->nextLow) > sys->maxHeapUsed)
        sys->maxHeapUsed = sys->heapSize - (sys->nextHigh - sys->nextLow);
    return p;
}

int VM_getchar(void) {
    int ch = getchar();
    if (ch == '\r')
        ch = '\n';

    return ch;
}

void VM_putchar(int ch) {
    if (ch == '\n')
        putchar('\r');

    putchar(ch);
}

void VM_flush(void) {
    fflush(stdout);
}

// VM_printf - formatted print
void VM_printf(const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    VM_vprintf(fmt, ap);
    va_end(ap);
}

void VM_vprintf(const char *fmt, va_list ap) {
    char buf[80], *p = buf;
    vsprintf(buf, fmt, ap);
    while (*p != '\0')
        VM_putchar(*p++);
}

void Abort(System_t *sys, const char *fmt, ...) {
    char buf[100], *p = buf;
    va_list ap;
    va_start(ap, fmt);
    VM_printf("error: ");
    vsprintf(buf, fmt, ap);
    while (*p != '\0')
        VM_putchar(*p++);
    VM_putchar('\n');
    va_end(ap);
    longjmp(sys->errorTarget, 1);
}
