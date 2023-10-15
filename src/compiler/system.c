/*
 * @system.c
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
#include <string.h>

#include "system.h"

// initialize the compiler
vm_context_t* system_init_context(uint8_t *freeSpace, size_t freeSize) {
    vm_context_t *sys = (vm_context_t*) freeSpace;
    if (freeSize < sizeof(vm_context_t))
        return NULL;
    sys->freeSpace = freeSpace + sizeof(vm_context_t);
    sys->freeTop = freeSpace + freeSize;
    sys->nextLow = sys->freeSpace;
    sys->nextHigh = sys->freeTop;
    sys->heapSize = sys->freeTop - sys->freeSpace;
    sys->maxHeapUsed = 0;
    return sys;
}

// allocate high memory from the heap
void* system_allocate_high_memory(vm_context_t *sys, size_t size) {
    size = (size + ALIGN_MASK) & ~ALIGN_MASK;
    if (sys->nextHigh - size < sys->nextLow)
        vm_system_abort(sys, "insufficient memory");
    sys->nextHigh -= size;
    if (sys->heapSize - (sys->nextHigh - sys->nextLow) > sys->maxHeapUsed)
        sys->maxHeapUsed = sys->heapSize - (sys->nextHigh - sys->nextLow);
    return sys->nextHigh;
}

// GetMainSource - get the main source
void system_get_main_source(System_line_t *sys, GetLineHandler **pGetLine, void **pGetLineCookie) {
    *pGetLine = sys->getLine;
    *pGetLineCookie = sys->getLineCookie;
}

// SetMainSource - set the main source
void system_set_main_source(System_line_t *sys, GetLineHandler *getLine, void *getLineCookie) {
    sys->getLine = getLine;
    sys->getLineCookie = getLineCookie;
}

// GetLine - get the next input line
int system_get_line(System_line_t *sys, int *pLineNumber) {
    if (!(*sys->getLine)(sys->lineBuf, sizeof(sys->lineBuf) - 1, pLineNumber, sys->getLineCookie))
        return VMFALSE;
    sys->linePtr = sys->lineBuf;
    return VMTRUE;
}

void* system_fs_open(vm_context_t *sys, const char *name, const char *mode) {
    return (void*) fopen(name, mode);
}

char* system_fs_getline(char *buf, int size, void *fp) {
    return fgets(buf, size, (FILE*) fp);
}

void system_fs_close(void *fp) {
    fclose((FILE*) fp);
}

int system_fs_opendir(const char *path, VMDIR_t *dir) {
    if (!(dir->dirp = opendir(path)))
        return -1;
    return 0;
}

int system_fs_readdir(VMDIR_t *dir, VMDIRENT_t *entry) {
    struct dirent *ansi_entry;

    if (!(ansi_entry = readdir(dir->dirp)))
        return -1;

    strcpy(entry->name, ansi_entry->d_name);

    return 0;
}

void system_fs_closedir(VMDIR_t *dir) {
    closedir(dir->dirp);
}
