/*
 * @vmsystem.h
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

#ifndef __VMSYSTEM_H__
#define __VMSYSTEM_H__

#include <stdarg.h>
#include <setjmp.h>

#include "vmtypes.h"

// system context
typedef struct vm_context_s {
           jmp_buf errorTarget;      // error target
           uint8_t *freeSpace;       // base of free space
           uint8_t *freeNext;        // next free space available
           uint8_t *freeTop;         // top of free space
           uint8_t *nextHigh;        // next high memory heap space location
           uint8_t *nextLow;         // next low memory heap space location
            size_t heapSize;         // size of heap space in bytes
            size_t maxHeapUsed;      // maximum amount of heap space allocated so far
} vm_context_t;

void* vm_allocate_low_memory(vm_context_t *sys, size_t size);
 void vm_system_abort(vm_context_t *sys, const char *fmt, ...);

  int vm_getchar(void);
 void vm_printf(const char *fmt, ...);
 void vm_vprintf(const char *fmt, va_list ap);
 void vm_putchar(int ch);
 void vm_flush(void);

#endif /* __VMSYSTEM_H__ */
