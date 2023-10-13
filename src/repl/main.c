/* @main.c
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
#include <stdlib.h>
#include <stdarg.h>
#include <ctype.h>
#include <string.h>

#include "edit.h"
#include "compile.h"
#include "system.h"

// fastspin memory map:
// There is a command line switch (-H) to specify the starting address, but the default is 0.
// First comes whatever COG code is needed (very little for the P2, just enough to bootstrap).
// Then comes the hubexec code. Then comes the data (including all variables). The heap (the
// size of which is given by HEAPSIZE) is part of that data. After that comes the stack, which
// grows upwards towards the end of memory. You can access the stack pointer by the variable
// "sp" in inline assembly, or by calling __getsp() in a high level language. The stuff above
// that is "free" (as long as you don't need a deeper stack, of course).


#define WORKSPACE_SIZE  (64 * 1024)
uint8_t workspace[WORKSPACE_SIZE];

static char* GetConsoleLine(char *buf, int size, int *pLineNumber, void *cookie);

int main(int argc, char *argv[]) {
    System_t *sys = InitSystem(workspace, sizeof(workspace));

    if (sys) {
        sys->getLine = GetConsoleLine;
        EditWorkspace(sys);
    }
    return 0;
}

static char *GetConsoleLine(char *buf, int size, int *pLineNumber, void *cookie) {
    int i = 0;
    while (i < size - 1) {
        int ch = VM_getchar();
        if (ch == '\n') {
            buf[i++] = '\n';
            VM_putchar('\n');
            break;
        }
        else if (ch == '\b' || ch == 0x7f) {
            if (i > 0) {
                if (ch == 0x7f)
                    VM_putchar('\b');
                VM_putchar(' ');
                VM_putchar('\b');
                VM_flush();
                --i;
            }
        }
        else {
            buf[i++] = ch;
            VM_flush();
        }
    }
    buf[i] = '\0';
    return buf;
}
