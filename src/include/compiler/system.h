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

// program limits
#define MAXLINE  128

// line input handler
typedef char* GetLineHandler(char *buf, int len, int *pLineNumber, void *cookie);

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


typedef struct System_line_s {
    GetLineHandler *getLine;         // function to get a line from the source program
              void *getLineCookie;   // cookie for the rewind and getLine functions
              char lineBuf[MAXLINE]; // current input line
              char *linePtr;         // pointer to the current character
} System_line_t;

// code generator context
typedef struct GenerateContext_s {
    vm_context_t *sys;
         uint8_t *codeBuf;
        uint32_t code_len;
} GenerateContext_t;

vm_context_t* system_init_context(uint8_t *freeSpace, size_t freeSize);
        void* system_allocate_high_memory(vm_context_t *sys, size_t size);

         void system_get_main_source(System_line_t *sys, GetLineHandler **pGetLine, void **pGetLineCookie);
         void system_set_main_source(System_line_t *sys, GetLineHandler *getLine, void *getLineCookie);
          int system_get_line(System_line_t *sys, int *pLineNumber);

        void* system_fs_open(vm_context_t *sys, const char *name, const char *mode);
        char* system_fs_getline(char *buf, int size, void *fp);
         void system_fs_close(void *fp);
          int system_fs_opendir(const char *path, VMDIR_t *dir);
          int system_fs_readdir(VMDIR_t *dir, VMDIRENT_t *entry);
         void system_fs_closedir(VMDIR_t *dir);

#endif
