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

// program limits
#define MAXLINE  128

// forward type definitions
typedef struct System_s System_t;

// line input handler
typedef char* GetLineHandler(char *buf, int len, int *pLineNumber, void *cookie);

// system context
struct System_s {
           jmp_buf errorTarget;      // error target
           uint8_t *freeSpace;       // base of free space
           uint8_t *freeNext;        // next free space available
           uint8_t *freeTop;         // top of free space
           uint8_t *nextHigh;        // next high memory heap space location
           uint8_t *nextLow;         // next low memory heap space location
            size_t heapSize;         // size of heap space in bytes
            size_t maxHeapUsed;      // maximum amount of heap space allocated so far
    GetLineHandler *getLine;         // function to get a line from the source program
              void *getLineCookie;   // cookie for the rewind and getLine functions
              char lineBuf[MAXLINE]; // current input line
              char *linePtr;         // pointer to the current character
};

void* AllocateLowMemory(System_t *sys, size_t size);
 void Abort(System_t *sys, const char *fmt, ...);

  int VM_getchar(void);
 void VM_printf(const char *fmt, ...);
 void VM_vprintf(const char *fmt, va_list ap);
 void VM_putchar(int ch);
 void VM_flush(void);

#endif /* __VMSYSTEM_H__ */
