/*
 * @system.h
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

#ifndef __SYSTEM_H__
#define __SYSTEM_H__

#include <dirent.h>
#include <stdarg.h>
#include <setjmp.h>

#include "vmtypes.h"
#include "vmsystem.h"

// ANSI_FILE_IO
typedef FILE VMFILE;

#define VM_fopen    fopen
#define VM_fclose   fclose
#define VM_fgets    fgets
#define VM_fputs    fputs

typedef struct {
    DIR *dirp;
} VMDIR_t;

typedef struct {
    char name[FILENAME_MAX];
} VMDIRENT_t;

// code generator context
typedef struct GenerateContext_s GenerateContext_t;
struct GenerateContext_s {
    System_t *sys;
     uint8_t *codeBuf;
};

System_t* InitSystem(uint8_t *freeSpace, size_t freeSize);
    void* AllocateHighMemory(System_t *sys, size_t size);

    void GetMainSource(System_t *sys, GetLineHandler **pGetLine, void **pGetLineCookie);
    void SetMainSource(System_t *sys, GetLineHandler *getLine, void *getLineCookie);
     int GetLine(System_t *sys, int *pLineNumber);

    void VM_sysinit(int argc, char *argv[]);
   void* VM_open(System_t *sys, const char *name, const char *mode);
   char* VM_getline(char *buf, int size, void *fp);
    void VM_close(void *fp);
     int VM_opendir(const char *path, VMDIR_t *dir);
     int VM_readdir(VMDIR_t *dir, VMDIRENT_t *entry);
    void VM_closedir(VMDIR_t *dir);

#endif